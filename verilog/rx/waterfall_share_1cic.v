/*
--------------------------------------------------------------------------------
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
Boston, MA  02110-1301, USA.
--------------------------------------------------------------------------------
*/

// Copyright (c) 2014-2026 John Seamons, ZL4VO/KF6VO

`timescale 1ns / 100ps

module WATERFALL_SHARE_1CIC
    #(parameter BUFSIZE = "required", parameter IN_WIDTH = "required")
    (
        input  wire		   adc_clk,
        input  wire signed [IN_WIDTH-1:0] adc_data,
        
        input  wire		   wf_sel_C,            // for items related to WATERFALL_1CIC: 0..V_WF_CHANS-1
        input  wire		   wf_sel2_C,           // for items related to WF_CICF_MEM:    0..V_RX_CHANS-1
        input  wire		   rd_i_A,
        input  wire		   rd_q_A,
        output wire		   wf_decim_zero_A,
        output wire		   wf_full_C,
        output wire		   wf_full_pulse_C,
        output wire [15:0] wf_dout_A,

        input  wire        cpu_clk,
        input  wire [31:0] freeze_tos_A,
    
        input  wire        rst_wf_wr_A,         // qualified with wf_sel_C below
        input  wire        rst_wf_rd_A,         // qualified with wf_sel2_C below
        input  wire        set_wf_freqH_C,      // qualified with wf_sel_C below
        input  wire        set_wf_freqL_C,      // qualified with wf_sel_C below
        input  wire        set_wf_decim_C,      // qualified with wf_sel_C below
        
        input  wire        ch
    );

`include "kiwi.gen.vh"

	wire reset_wr_A = wf_sel_C  && rst_wf_wr_A;
	wire reset_rd_A = wf_sel2_C && rst_wf_rd_A;

	reg signed [47:0] wf_phase_inc;
	wire set_wf_freqH_A, set_wf_freqL_A;

	SYNC_PULSE set_freqH_inst (.in_clk(cpu_clk), .in(wf_sel_C && set_wf_freqH_C), .out_clk(adc_clk), .out(set_wf_freqH_A));
	SYNC_PULSE set_freqL_inst (.in_clk(cpu_clk), .in(wf_sel_C && set_wf_freqL_C), .out_clk(adc_clk), .out(set_wf_freqL_A));

    always @ (posedge adc_clk)
    begin
        if (set_wf_freqH_A) wf_phase_inc[16 +:32] <= freeze_tos_A;
        if (set_wf_freqL_A) wf_phase_inc[ 0 +:16] <= freeze_tos_A;
    end

	wire signed [WF1_BITS-1:0] wf_mix_i, wf_mix_q;

    IQ_MIXER #(.IN_WIDTH(IN_WIDTH), .OUT_WIDTH(WF1_BITS))
        wf_mixer (
            .clk		(adc_clk),
            .phase_inc	(wf_phase_inc),
            .in_data	(adc_data),
            .out_i		(wf_mix_i),
            .out_q		(wf_mix_q)
        );
	
	wire set_wf_decim_A;
	SYNC_PULSE set_decim_inst (.in_clk(cpu_clk), .in(wf_sel_C && set_wf_decim_C), .out_clk(adc_clk), .out(set_wf_decim_A));

	localparam MD = max(1, clog2(WF_1CIC_MAXD + 1));    // +1 because need to represent WF_1CIC_MAXD, not WF_1CIC_MAXD-1
	// see freeze_tos_A[] below
	// assert_cond(WF_1CIC_MAXD <= 32768);
	// assert_cond(MD <= 16);
	//wire [MD-1:0] md = 0; how_big(.p(md));

	reg [MD-1:0] decim;
	assign wf_decim_zero_A = (decim == 0);
    always @ (posedge adc_clk)
        if (set_wf_decim_A)
        	decim <= freeze_tos_A[0 +:MD];

    wire wf_cic_avail;
    wire [WFO_BITS-1:0] wf_cic_out_i, wf_cic_out_q;
    
    // NB: for N=pow2, M=1: N * log2(R) == log2(pow(R*M, N)), N=#stages, R=decim
    localparam WF1_GROWTH = WF1_STAGES * clog2(WF_1CIC_MAXD);
    //wire [WF1_GROWTH-1:0] wf1_growth = 0; how_big(.p(wf1_growth));
    
    // decim = 1 .. WF_1CIC_MAXD

    cic_prune_var #(.INC_FILE("wf1"), .STAGES(WF1_STAGES), .DECIM_TYPE(-WF_1CIC_MAXD), .GROWTH(WF1_GROWTH), .IN_WIDTH(WF1_BITS), .OUT_WIDTH(WFO_BITS))
    wf_cic_i(
        .clock			(adc_clk),
        .reset			(reset_wr_A),
        .decimation		(decim),
        .in_strobe		(1'b1),
        .out_strobe		(wf_cic_avail),
        .in_data		(wf_mix_i),
        .out_data		(wf_cic_out_i)
    );

    cic_prune_var #(.INC_FILE("wf1"), .STAGES(WF1_STAGES), .DECIM_TYPE(-WF_1CIC_MAXD), .GROWTH(WF1_GROWTH), .IN_WIDTH(WF1_BITS), .OUT_WIDTH(WFO_BITS))
    wf_cic_q(
        .clock			(adc_clk),
        .reset			(reset_wr_A),
        .decimation		(decim),
        .in_strobe		(1'b1),
        .out_strobe		(),
        .in_data		(wf_mix_q),
        .out_data		(wf_cic_out_q)
    );
    
    wire wf_full_A, wf_full_pulse_A;

//`define TEST_DATA
`ifdef TEST_DATA
    reg [12:0] test_data;
    always @ (posedge adc_clk) begin
        if (reset_wr_A) test_data <= 0;
        else
        if (wf_cic_avail) test_data <= test_data + 1'b1;
    end
`endif

    WF_SAMPLER_4K_8K_32B #(.BUFSIZE(BUFSIZE)) wf_samp(
        .wr_clk			(adc_clk),
        .wr_rst			(reset_wr_A),
        .wr				(wf_cic_avail),
`ifdef TEST_DATA
        .wr_i			({ch, 2'b00, test_data}),
        .wr_q			({ch, 2'b01, (test_data ^ 13'h1fff)}),
`else
        .wr_i			(wf_cic_out_i),
        .wr_q			(wf_cic_out_q),
`endif
        // o
        .wr_full        (wf_full_A),
        .wr_full_pulse  (wf_full_pulse_A),
        
        .rd_clk			(adc_clk),
        .rd_rst			(reset_rd_A),
        .rd_i			(rd_i_A),
        .rd_q			(rd_q_A),
        // o
        .rd_iq			(wf_dout_A)
    );

    SYNC_WIRE  sync_wf_full  (.in(wf_full_A), .out_clk(cpu_clk), .out(wf_full_C));
    SYNC_PULSE sync_wf_pulse (.in_clk(adc_clk), .in(wf_full_pulse_A), .out_clk(cpu_clk), .out(wf_full_pulse_C));

endmodule
