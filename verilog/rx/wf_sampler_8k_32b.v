// Copyright (c) 2014-2025 John Seamons, ZL4VO/KF6VO

`timescale 1ns / 100ps

// IQ sampler, 8K x 2 x 16-bit
// clock domains: fully isolated

module WF_SAMPLER_8K_32B
	#(parameter A_MSB = 12)
    (
        input  wire wr_clk,
        input  wire wr_rst,
        input  wire wr_continuous,		// if true, the wr_clk side doesn't stop capturing when buffer full
        input  wire wr,
        input  wire [15:0] wr_i,
        input  wire [15:0] wr_q,
    
        input  wire rd_clk,
        input  wire rd_rst,
        input  wire rd_sync,			// set rd_addr to (wr_addr + rd_offset), crucial look-ahead to previous buffer contents
        input  wire rd_i,
        input  wire rd_q,
        input  wire [11:0] rd_offset,
        output wire [15:0] rd_iq
    );
        
	// wr_clk side
    reg [A_MSB:0] wr_addr;
    reg           wr_full;
    wire	      wr_en = wr_continuous? wr : (wr && ~wr_full);
    
    always @ (posedge wr_clk)
    begin
        if (wr_rst) {wr_addr, wr_full} <= 0;
        else
        if (wr_en) {wr_full, wr_addr} <= wr_addr + 1;
    end
	
    wire [31:0] wr_diq = { wr_i[15 -:16], wr_q[15 -:16] };

	// rd_clk side
    reg [A_MSB:0]  rd_addr;
	wire [A_MSB:0] rd_next = rd_addr + rd_q;
	
	wire [A_MSB:0] sync_wr_addr;

    // continuously sync wr_addr => sync_wr_addr
	SYNC_REG #(.WIDTH(A_MSB+1)) sync_wr_addr_inst (
	    .in_strobe(1),      .in_reg(wr_addr),           .in_clk(wr_clk),
	    .out_strobe(),      .out_reg(sync_wr_addr),     .out_clk(rd_clk)
	);
	
    always @ (posedge rd_clk)
        if (rd_rst)
            rd_addr <= 0;
        else
        if (rd_sync)
        	rd_addr <= sync_wr_addr + rd_offset;
        else begin
            rd_addr <= rd_next;
		end
	
	wire [31:0] rd_diq;
	assign rd_iq = rd_i? rd_diq[31 -:16] : rd_diq[15 -:16];
	
	// done as an 8kx32b (7.5 BRAM) rather than 8kx16bx2 (8 BRAM)
	ipcore_bram_8k_32b iq_samp (
		.clka	(wr_clk),			.clkb	(rd_clk),
		.wea	(wr_en),
		.addra	(wr_addr),			.addrb	(rd_next),
		.dina	(wr_diq),			.doutb	(rd_diq)
	);
         
endmodule
