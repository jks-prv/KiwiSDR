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

module WF_CICF_MEM
    #(parameter WIDTH = "required")
    (
        input  wire         adc_clk,

        input  wire signed [WIDTH-1:0] wf_data_to_cicf,
        input  wire         wf_decim_zero_A,
        output wire         rd_data_i,
        output wire         rd_data_q,

        input  wire         get_wf_samp_i_C,
        input  wire         get_wf_samp_q_C,
        output wire [15:0]  wf_dout_C,

        input  wire         cpu_clk,
        input  wire [31:0]  freeze_tos_A,
        output reg  [15:0]  debug,
        
        input  wire         set_wf_fir_tap_C,
        input  wire         rst_wf_fir_A,
        input  wire         rst_wf_fir_C,
        input  wire         run_wf_fir_A
    );
    
`include "kiwi.gen.vh"

    //////////////////////////////////////////////////////////////////////////
    // WF CIC FIR filter
    //////////////////////////////////////////////////////////////////////////

	wire set_wf_fir_tap_A;
	SYNC_PULSE set_fir_tap_inst (.in_clk(cpu_clk), .in(set_wf_fir_tap_C), .out_clk(adc_clk), .out(set_wf_fir_tap_A));

    // WF samp BRAM is 8k IQ samps.
    // run_wf_fir_A asserted 4 times.
    // Loops 2046 times for each assertion, processing IQ pairs (2046*4 = 8184)
    // But because of decim-by-2 only outputs 1023 IQ pairs. Which is size of SPI buffer.
    
    localparam LOOP_CT_Z0 = 1023;   // processes IQ pair for each loop count

    // 1023*2(IQ) = 2046, *4 = 8184B, 4092W
    localparam LOOP_CT = 2046;      // processes IQ pair for each loop count
    localparam LOOP_MSB = 12;

    reg [LOOP_MSB:0] fir_ct;
    reg run, in_strobe;
    reg [2:0] wait_output;
    wire out_strobe;
    reg decim_by_2;
    wire [15:0] wf_data_from_cicf;
    reg [15:0] copy_buf;
    reg [15:0] dbg_ct;

    reg wr_en_z0, rd_data_i_z0, rd_data_q_z0;
    wire wr_en_fir, rd_data_i_fir, rd_data_q_fir;
    wire wr_en = wf_decim_zero_A? wr_en_z0 : wr_en_fir;
    assign rd_data_i = wf_decim_zero_A? rd_data_i_z0 : rd_data_i_fir;
    assign rd_data_q = wf_decim_zero_A? rd_data_q_z0 : rd_data_q_fir;

    always @ (posedge adc_clk)
    begin
        if (rst_wf_fir_A) begin
            {run, wait_output, in_strobe, wr_en_z0, rd_data_i_z0, rd_data_q_z0} <= 0;
        end else
        if (run_wf_fir_A) begin
            {fir_ct, dbg_ct, wait_output} <= 0;
            run <= 1;
        end else
        if (run) begin
            // z0 z1 non-CICF copy loop
            if (wf_decim_zero_A) begin
                if (fir_ct < LOOP_CT_Z0) begin
                    if (wait_output == 0) begin
                        rd_data_i_z0 <= 1;      // NB: one clk before reading wf_data_to_cicf
                        wr_en_z0 <= 0;
                        wait_output <= 1;
                    end else
                    if (wait_output == 1) begin
                        rd_data_i_z0 <= 0;
                        copy_buf <= wf_data_to_cicf;    // read I
                        wait_output <= 2;
                    end else
                    if (wait_output == 2) begin
                        rd_data_q_z0 <= 1;
                        wr_en_z0 <= 1;                  // write I
                        wait_output <= 3;
                    end else
                    if (wait_output == 3) begin
                        rd_data_q_z0 <= 0;
                        copy_buf <= wf_data_to_cicf;    // read Q
                        wr_en_z0 <= 0;
                        wait_output <= 4;
                    end else
                    if (wait_output == 4) begin
                        wr_en_z0 <= 1;                  // write Q
                        wait_output <= 0;
                        fir_ct <= fir_ct + 1'b1;
                    end
                end else begin
                    wr_en_z0 <= 0;
                    run <= 0;
                end
            end else begin
                // z2+ CICF copy loop
                if (fir_ct < LOOP_CT) begin
                    if (wait_output == 0) begin
                        debug <= dbg_ct;
                        dbg_ct <= dbg_ct + 1'b1;
                        in_strobe <= 1;     // pulse in_strobe
                        wait_output <= 1;
                    end else
                    if (wait_output == 1) begin
                        if (out_strobe) begin
                            wait_output <= 2;
                        end
                        debug <= dbg_ct;
                        dbg_ct <= dbg_ct + 1'b1;
                        in_strobe <= 0;
                    end else
                    if (wait_output == 2) begin
                        debug <= dbg_ct;
                        dbg_ct <= dbg_ct + 1'b1;
                        wait_output <= 0;
                        fir_ct <= fir_ct + 1'b1;
                    end
                end else begin
                    run <= 0;
                end
            end
        end else begin
            // else do nothing until run again
            {run, wait_output, in_strobe, wr_en_z0, rd_data_i_z0, rd_data_q_z0} <= 0;
        end
    end
    
    FIR_IQ_WF #(.WIDTH(WIDTH)) wf_cicf (
		.adc_clk        (adc_clk),
		.reset			(rst_wf_fir_A),
		
		.in_strobe      (in_strobe),
		.in_data		(wf_data_to_cicf),
		// o
		.rd_data_i		(rd_data_i_fir),
		.rd_data_q		(rd_data_q_fir),
		.wr_en          (wr_en_fir),
		
		// data out to output buffer
		// o
		.out_strobe     (out_strobe),       // takes NTAPS + overhead after in_strobe
		.out_data		(wf_data_from_cicf),
		
		.freeze_tos_A   (freeze_tos_A),
		.set_tap_A      (set_wf_fir_tap_A)
    );


    //////////////////////////////////////////////////////////////////////////
    // WF output buffer
    //////////////////////////////////////////////////////////////////////////

    wire [15:0] wf_data_to_obuf = wf_decim_zero_A? copy_buf : wf_data_from_cicf;

    // 1k x 2(IQ) x 16b = 32kb = 2k x 16b
    // buffer counters wrap around since only 1023 IQ samples (not 1024)
    // are processed by each of the 4x CmdGetWFSamples from the wf chunk loop
    localparam _2K = 10;
    reg  [_2K:0] wr_addr;
    //`define DEBUG_XFER
    `ifdef DEBUG_XFER
        reg iq;
    `endif
	
    always @ (posedge adc_clk)
        if (rst_wf_fir_A) begin
            wr_addr <= 0;
            `ifdef DEBUG_XFER
                iq <= 0;
            `endif
        end else begin 
            if (wr_en) begin
                wr_addr <= wr_addr + 1'b1;
                `ifdef DEBUG_XFER
                    iq <= iq ^ 1'b1;
                `endif
            end
        end

    reg  [_2K:0] rd_addr;
    wire [_2K:0] rd_a;

`define ADDR_PRE
`ifdef ADDR_PRE
	wire [_2K:0] rd_next = rd_addr + ((get_wf_samp_i_C || get_wf_samp_q_C)? 1'b1 : 1'b0);
    assign rd_a = rd_next;
`else
    assign rd_a = rd_addr;
`endif

    always @ (posedge cpu_clk)
        if (rst_wf_fir_C) begin
            rd_addr <= 0;
        end else begin
`ifdef ADDR_PRE
            rd_addr <= rd_next;
`else
            if (get_wf_samp_i_C || get_wf_samp_q_C)
                rd_addr <= rd_addr + 1'b1;
`endif
		end

    `ifdef DEBUG_XFER
        wire [15:0] dina = {fir_ct[3:0], iq, wr_addr};
    `else
        wire [15:0] dina = wf_data_to_obuf;
    `endif

	ipcore_bram_2k_16b iq_out_samp (
		.clka	(adc_clk),      .clkb	(cpu_clk),
		.wea	(wr_en),
		.addra	(wr_addr),      .addrb	(rd_a),
		.dina	(dina),         .doutb	(wf_dout_C)
	);

endmodule
