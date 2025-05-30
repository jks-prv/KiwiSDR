
; ============================================================================
; Homemade GPS Receiver
; Copyright (C) 2013 Andrew Holme
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.
;
; http://www.holmea.demon.co.uk/GPS/Main.htm
; ============================================================================

; Copyright (c) 2014-2025 John Seamons, ZL4VO/KF6VO


; ============================================================================
; receiver
; ============================================================================

nrx_samps:		u16		0

#if SND_SEQ_CHECK
rx_seq:			u16		0
#endif
				// this routing called from main loop at audio buffer flip rate (i.e. every NRX_SAMPS audio samples)
				// when polling with rdReg GET_RX_SRQ finds rx_srq asserted
RX_Buffer:
				// if nrx_samps has not been set yet hardware will not interrupt at correct rate
				push	nrx_samps
				fetch16
				brZ		not_init
				
				push	CTRL_SND_INTR
				call	ctrl_set			; signal the interrupt
				
#if SND_SEQ_CHECK
				// increment seq
                push	rx_seq
                incr16
				pop
#endif
not_init:		ret

CmdGetRX:
                rdReg	HOST_RX				; nrx_samps_total - 1
				wrEvt	HOST_RST

				push	CTRL_SND_INTR
				call	ctrl_clr			; clear the interrupt as a side-effect

#if SND_SEQ_CHECK
				push	rx_seq				; &rx_seq
				fetch16						; rx_seq
				push	0x0ff0
				wrReg	HOST_TX
				wrReg	HOST_TX
#endif
                                            ; cnt = nrx_samps_total - 1
                to_loop                     ;
                ALIGN
rx_loop:
                wrEvt2	GET_RX_SAMP			; move i
				wrEvt2	GET_RX_SAMP			; move q
				wrEvtL	GET_RX_SAMP_LOOP    ; move iq3
				// wrEvtL will automatically loop to rx_loop

                // tail information: ticks, stored/current buffer counters
				wrEvt2	GET_RX_SAMP			; move ticks[3]
				wrEvt2	GET_RX_SAMP
				wrEvt2	GET_RX_SAMP
				
				wrEvt2	GET_RX_SAMP         ; move stored buffer counter
				wrEvt2  RX_GET_BUF_CTR      ; move current buffer counter
				ret

CmdSetRXNsamps:	rdReg	HOST_RX				; nsamps
				dup
				push	nrx_samps
				store16
				pop							; nsamps
				
				FreezeTOS
                wrReg2	SET_RX_NSAMPS		;
                
                wrEvt2  RX_BUFFER_RST       ; reset read/write pointers, buffer counter
                ret

CmdSetRXFreq:	rdReg	HOST_RX				; rx#
				wrReg2	SET_RX_CHAN			;
                RdReg32	HOST_RX				; freqH
				FreezeTOS
                wrReg2	SET_RX_FREQ			;
                B2B_FreezeTOS               ; delay so back-to-back FreezeTOS works
                rdReg	HOST_RX				; freqL
				FreezeTOS
                wrReg2	SET_RX_FREQ | FREQ_L
                ret

CmdClrRXOvfl:
				wrEvt2	CLR_RX_OVFL
				ret

CmdSetGenFreq:
#if	USE_GEN
				rdReg	HOST_RX				; rx#   ignored: gen is applied in place of ADC data
                RdReg32	HOST_RX				; rx# genH
				FreezeTOS
                wrReg2	SET_GEN_FREQ        ; rx#
                B2B_FreezeTOS               ; delay so back-to-back FreezeTOS works
                rdReg	HOST_RX				; genL
				FreezeTOS
                wrReg2	SET_RX_FREQ | FREQ_L ; rx#
                drop.r                      ;
#else
                ret
#endif

CmdSetGenAttn:
#if	USE_GEN
				rdReg	HOST_RX				; wparam
                RdReg32	HOST_RX				; wparam lparam
				FreezeTOS
                wrReg2	SET_GEN_ATTN		; wparam
                drop.r
#else
                ret
#endif

CmdSetOVMask:
				rdReg	HOST_RX				; wparam
                RdReg32	HOST_RX				; wparam lparam
				FreezeTOS
                wrReg   SET_CNT_MASK		; wparam
                drop.r

CmdGetADCCtr:
				wrEvt	HOST_RST
				rdReg	GET_ADC_CTR0        ; adc_count[15:0]
				wrReg	HOST_TX
				rdReg	GET_ADC_CTR1        ; adc_count[31:16]
				wrReg	HOST_TX
                ret

CmdSetADCLvl:
                rdReg	HOST_RX				; adc_level[13:0]
				FreezeTOS
				wrReg	SET_ADC_LVL
                ret


; ============================================================================
; waterfall
; ============================================================================

CmdWFReset:	
				rdReg	HOST_RX				; wf_chan
				wrReg2	SET_WF_CHAN			;
				rdReg	HOST_RX             ; WF_SAMP_*
				FreezeTOS
				wrReg2	WF_SAMPLER_RST
            	ret

CmdGetWFSamples:
				rdReg	HOST_RX				; wf_chan
				wrReg2	SET_WF_CHAN			;
getWFSamples2:
				wrEvt	HOST_RST
                push    nwf_samps_m1        ; &nwf_samps_m1
                fetch16                     ; nwf_samps_m1
                to_loop                     ;
                ALIGN
wf_loop:
				wrEvt2	GET_WF_SAMP_I
				wrEvtL	GET_WF_SAMP_Q_LOOP
				// wrEvtL will automatically loop to wf_loop
				ret

CmdGetWFContSamps:
				rdReg	HOST_RX				; wf_chan
				wrReg2	SET_WF_CHAN			;
				push	WF_SAMP_SYNC | WF_SAMP_CONTIN
				FreezeTOS
				wrReg2	WF_SAMPLER_RST
				br		getWFSamples2

nwf_samps_m1:   u16		0

CmdSetWFOffset:	
				rdReg	HOST_RX				; wf_chan
				wrReg2	SET_WF_CHAN         ;
                rdReg	HOST_RX				; offset
				wrReg2	SET_WF_OFFSET		;
				rdReg	HOST_RX				; nwf_samps_m1
				push	nwf_samps_m1        ; nwf_samps_m1 &nwf_samps_m1
				store16                     ; &nwf_samps_m1
				pop.r                       ;

CmdSetWFFreq:	rdReg	HOST_RX				; wf_chan
				wrReg2	SET_WF_CHAN			;
                RdReg32	HOST_RX				; freqH
				FreezeTOS
                wrReg2	SET_WF_FREQ			;
                B2B_FreezeTOS               ; delay so back-to-back FreezeTOS works
                rdReg	HOST_RX				; freqL
				FreezeTOS
                wrReg2	SET_WF_FREQ | FREQ_L
				ret

CmdSetWFDecim:	
				rdReg	HOST_RX				; wf_chan
				wrReg2	SET_WF_CHAN			;
                RdReg32	HOST_RX				; lparam
				FreezeTOS
				wrReg2	SET_WF_DECIM		;
				ret
