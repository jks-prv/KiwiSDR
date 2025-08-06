//////////////////////////////////////////////////////////////////////
// cuteSDR.h: Common data type declarations
//////////////////////////////////////////////////////////////////////

#pragma once

// This is here instead of fastfir.h because of problems with recursive definitions.

// Needs to be 1024 (was 2048) otherwise audio packets aren't output frequently enough
// to prevent drops at client.

#define CONV_FFT_SIZE				1024		// must be power of 2
#define CONV_FFT_TO_OUTBUF_RATIO	2
#define MAX_NRX_SAMPS               NRX_SAMPS_CHANS(3)      // i.e. 226 with rx3wf3

#define FASTFIR_OUTBUF_SIZE (CONV_FFT_SIZE / CONV_FFT_TO_OUTBUF_RATIO)
