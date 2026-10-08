// Copyright (c) 2014-2026 John Seamons, ZL4VO/KF6VO

`timescale 1ns / 100ps

// IQ sampler, 4K/8K x 2 x 16-bit
// clock domains: fully isolated

module WF_SAMPLER_4K_8K_32B
	#(parameter BUFSIZE = "required")
    (
        input  wire wr_clk,
        input  wire wr_rst,
        input  wire wr,
        input  wire [15:0] wr_i,
        input  wire [15:0] wr_q,
        output reg  wr_full,
        output reg  wr_full_pulse,
    
        input  wire rd_clk,
        input  wire rd_rst,
        input  wire rd_i,
        input  wire rd_q,
        output wire [15:0] rd_iq
    );
    
`include "kiwi.gen.vh"

    localparam A_MSB = clog2(BUFSIZE) - 1;
        
	// wr_clk side
    reg [A_MSB:0] wr_addr;
    wire	      wr_en = wr && ~wr_full;
    
    localparam RISE = 2'b10;
    reg wr_full_prev;
    
    always @ (posedge wr_clk)
    begin
        if (wr_rst) {wr_addr, wr_full, wr_full_prev} <= 0;
        else
        if (wr_en) {wr_full, wr_addr} <= wr_addr + 1;
        
        wr_full_pulse <= {wr_full, wr_full_prev} == RISE;
        wr_full_prev  <= wr_full;
    end
	
    wire [31:0] wr_diq = { wr_i[15 -:16], wr_q[15 -:16] };

	// rd_clk side
    reg [A_MSB:0]  rd_addr;
	wire [A_MSB:0] rd_next = rd_addr + rd_q;
	
    always @ (posedge rd_clk)
        if (rd_rst)
            rd_addr <= 0;
        else
            rd_addr <= rd_next;
	
	wire [31:0] rd_diq;
	assign rd_iq = rd_i? rd_diq[31 -:16] : rd_diq[15 -:16];
	
	generate
		if (BUFSIZE == 8192) begin
            // done as an 8kx32b (7.5 BRAM) rather than 8kx16bx2 (8 BRAM)
            ipcore_bram_8k_32b iq_samp (
                .clka	(wr_clk),			.clkb	(rd_clk),
                .wea	(wr_en),
                .addra	(wr_addr),			.addrb	(rd_next),
                .dina	(wr_diq),			.doutb	(rd_diq)
            );
        end else
		if (BUFSIZE == 4096) begin
            ipcore_bram_4k_32b iq_samp (
                .clka	(wr_clk),			.clkb	(rd_clk),
                .wea	(wr_en),
                .addra	(wr_addr),			.addrb	(rd_next),
                .dina	(wr_diq),			.doutb	(rd_diq)
            );
		end
	endgenerate
         
endmodule
