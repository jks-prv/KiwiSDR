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

// Copyright (c) 2023 Christoph Mayer, DL1CH
// Copyright (c) 2023-2026 John Seamons, ZL4VO/KF6VO

`timescale 1ns / 100ps

module FIR_IQ_WF
    #(parameter WIDTH = 0)
    (
        input  wire     adc_clk,
        input  wire     reset,

        input  wire     in_strobe,
        input  wire signed [WIDTH-1:0] in_data,
        output reg      rd_data_i,
        output reg      rd_data_q,
        output reg      wr_en,

        output reg      out_strobe,
        output reg signed [WIDTH-1:0] out_data,

        input  wire [31:0] freeze_tos_A,
        input  wire     set_tap_A
    );

`include "kiwi.gen.vh"

    // nominally ACCW(32) = WIDTH(16) + COEFF(16), uses 2 DSPs
    //localparam COEFF = WIDTH - 0;   // FIXME: lesser than WIDTH due to overflow issue?
    localparam COEFF = 16;   // FIXME: lesser than WIDTH due to overflow issue?
    localparam ACCW = WIDTH + COEFF;
    localparam ACCOUT = WIDTH + COEFF - 1;
    // WF_NTAPS needs to be of the form 2N + 1 due to odd symmetry of taps table
    localparam TAP_HSZ = (WF_NTAPS-1)/2;
    localparam TAP_MSB = $clog2(WF_NTAPS)-1;

    reg signed [ACCW-1:0]  accI, accQ;
    reg signed [WIDTH-1:0] bufI[WF_NTAPS-1:0], bufQ[WF_NTAPS-1:0];

//`define USE_INDATA

`define VAR_TAPS
`ifdef VAR_TAPS
    reg signed [COEFF-1:0] taps[TAP_HSZ:0];
    wire [TAP_MSB:0] tap_i = freeze_tos_A[24 +:(TAP_MSB+1)];
    always_ff @ (posedge adc_clk) begin
        if (set_tap_A) begin
            taps[tap_i] <= freeze_tos_A[COEFF-1:0];
        end
    end
`else
    wire signed [COEFF-1:0] taps[TAP_HSZ:0];
    assign taps[99]  = COEFF'('sh0000);
`endif

    reg decim_by_2;
    reg [TAP_MSB:0] tap;
    initial $display("#################### RX_CFG=%d WF_NTAPS=%d TAP_HSZ=%d TAP_MSB=%d $bits(tap)=%d ####################", RX_CFG, WF_NTAPS, TAP_HSZ, TAP_MSB, $bits(tap));

    /* Definitions for the FSM */
    typedef enum {
        STATE_IDLE,
        STATE_LATCH_INPUT_I,
        STATE_LATCH_INPUT_Q,
        STATE_COMPUTE_SUM,
        STATE_COPY_OUTPUT_I,
        STATE_COPY_OUTPUT_Q
        
    } state_t;
    state_t state, next_state;
    
    // simulator doesn't like initial when procedural assignment inside always_ff block?
`ifdef SYNTHESIS
    initial state = STATE_IDLE;
    initial decim_by_2 = 0;
`endif

    /* Implement the FSM */
    always_ff @(posedge adc_clk) begin
        if (reset) begin
            state <= STATE_IDLE;
        end else begin
            state <= next_state;
        end
    end

    /* Actions at state transitions */
    always_ff @(posedge adc_clk) begin
        if (reset) begin
            rd_data_i <= 0;
            rd_data_q <= 0;
            wr_en <= 0;
            out_strobe <= 0;
            decim_by_2 <= 0;
        end else begin
            if (next_state == STATE_IDLE) begin
                rd_data_i <= 1;     // rd_data_i is just a mux select so needs to be set early
                rd_data_q <= 0;
                wr_en <= 0;
            end else if (state == STATE_IDLE && next_state == STATE_LATCH_INPUT_I) begin
                // latch input
                bufI <= {bufI[WF_NTAPS-2:0], in_data};
                rd_data_i <= 0;
                rd_data_q <= 1;     // rd_data_q increments read address so set only once
            end else if (state == STATE_LATCH_INPUT_I && next_state == STATE_LATCH_INPUT_Q) begin
                // latch input
                bufQ <= {bufQ[WF_NTAPS-2:0], in_data};
                rd_data_q <= 0;
            end else if (state == STATE_LATCH_INPUT_Q && next_state == STATE_COMPUTE_SUM) begin
                // NOP
            end else if (state == STATE_COMPUTE_SUM && next_state == STATE_COPY_OUTPUT_I) begin
                if (!decim_by_2) begin
                    `ifdef USE_INDATA
                        out_data <= bufI[0];
                    `else
                        out_data <= accI[ACCOUT -:WIDTH];
                    `endif
                    wr_en <= 1;
                end
                out_strobe <= 1;
            end else if (state == STATE_COPY_OUTPUT_I && next_state == STATE_COPY_OUTPUT_Q) begin
                if (!decim_by_2) begin
                    `ifdef USE_INDATA
                        out_data <= bufQ[0];
                    `else
                        out_data <= accQ[ACCOUT -:WIDTH];
                    `endif
                    wr_en <= 1;
                end
                decim_by_2 <= decim_by_2 ^ 1'b1; // toggle
                out_strobe <= 0;
            end
        end
    end

    /* Accumulate the sum */
    always_ff @(posedge adc_clk) begin
        if (reset) begin
            tap <= 0;
            accI <= 0;
            accQ <= 0;
        end else begin
            if (state == STATE_COMPUTE_SUM && next_state != STATE_COPY_OUTPUT_I) begin
                if (tap <= TAP_HSZ) begin
                    accI <= accI + (bufI[tap] * taps[tap]);
                    accQ <= accQ + (bufQ[tap] * taps[tap]);
                end else begin
                    accI <= accI + (bufI[tap] * taps[WF_NTAPS-1-tap]);
                    accQ <= accQ + (bufQ[tap] * taps[WF_NTAPS-1-tap]);
                end
                tap <= tap + 1'b1;
            end else if (next_state == STATE_IDLE) begin
                // NB: careful not to zero acc[IQ] during STATE_COPY_OUTPUT_[IQ]
                tap <= 0;
                accI <= 0;
                accQ <= 0;
            end
        end
    end

    /* FSM next-state logic */
    always_comb begin
        next_state = state;

        unique case (state)
            STATE_IDLE: begin
                if (in_strobe) begin
                    next_state = STATE_LATCH_INPUT_I;
                end
            end

            STATE_LATCH_INPUT_I: begin
                next_state = STATE_LATCH_INPUT_Q;
            end

            STATE_LATCH_INPUT_Q: begin
                next_state = STATE_COMPUTE_SUM;
            end

            STATE_COMPUTE_SUM: begin
                 if (tap == WF_NTAPS) begin
                    next_state = STATE_COPY_OUTPUT_I;
                 end
            end

            STATE_COPY_OUTPUT_I: begin
                next_state = STATE_COPY_OUTPUT_Q;
            end

            STATE_COPY_OUTPUT_Q: begin
                next_state = STATE_IDLE;
            end

            default:
                next_state = STATE_IDLE;

        endcase
    end

endmodule
