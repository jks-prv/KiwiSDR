
// Copyright (c) 2024-2025 John Seamons, ZL4VO/KF6VO

`timescale 1ns / 100ps

module rx_audio_mem_test ();
    
`include "kiwi.gen.vh"

    reg adc_clk;
    localparam ADC_CLK_PERIOD = 1;
    initial adc_clk = 1'b1;
    always # (ADC_CLK_PERIOD/2.0)
        adc_clk = ~adc_clk;

    reg cpu_clk;
    localparam CPU_CLK_PERIOD = 1;
    initial cpu_clk = 1'b1;
    always # (CPU_CLK_PERIOD/2.0)
        cpu_clk = ~cpu_clk;

    reg [9:0] nrx_samps_A = N_SAMPS;
    reg [15:0] rx_en_A = 16'h0013;
    reg rx_avail_A;
    reg [47:0] ticks_latched_A = 48'h012345678ABCD;
    
    wire ser, rd_getI, rd_getQ;
    
    reg [V_RX_CHANS*16-1:0] rxn_din_A;
    initial rxn_din_A = 0;
    always # (ADC_CLK_PERIOD)
        rxn_din_A = rxn_din_A + 1;

    reg get_rx_srq_C, get_rx_samp_C, reset_bufs_C, get_buf_ctr_C;
    
    wire rx_rd_C;
    wire [15:0] rx_dout_C;

`define SHORT
`ifdef SHORT
    localparam N_SAMPS = 10'd680;
    localparam CYCLES = 4;
`else
    localparam N_SAMPS = 10'd672;           // 672 (0x2a0) = 42*(0+16)
    localparam ALL_CYCLES = 16;
    localparam CYCLES = (672/ALL_CYCLES+3); // 672/16 + ceil/slop
`endif

    localparam RX_EN = 1 * 3;
    localparam RX_IDLE = 7 * 1;
    localparam GAP = 8;

    integer i, j;
    initial begin
        #0.9;   // setup
        #1; reset_bufs_C = 1;
        #1; reset_bufs_C = 0;
        #2;
        
        for (i = 0; i < CYCLES; i++)
        begin
            for (j = 0; j < N_SAMPS; j++)
            begin
                #(1); rx_avail_A = 1;
                #(1); rx_avail_A = 0;
                #(RX_EN + RX_IDLE + GAP);
            end
        end
    end
    
    rx_audio_mem rx_audio_mem_inst (
		.adc_clk		(adc_clk),
		.nrx_samps      (nrx_samps_A),
		.rx_en_A        (rx_en_A),
		.rx_avail_A     (rx_avail_A),
		.rxn_din_A      (rxn_din_A),
		.ticks_A        (ticks_latched_A),
		// o
		.ser            (ser),
		.rd_getI        (rd_getI),
		.rd_getQ        (rd_getQ),

		.cpu_clk        (cpu_clk),
		.get_rx_srq_C   (get_rx_srq_C),
		.get_rx_samp_C  (get_rx_samp_C),
		.reset_bufs_C   (reset_bufs_C),
		.get_buf_ctr_C  (get_buf_ctr_C),
		// o
		.rx_rd_C        (rx_rd_C),
		.rx_dout_C      (rx_dout_C)
    );

endmodule
