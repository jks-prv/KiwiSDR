/*
This software is part of libcsdr, a set of simple DSP routines for
Software Defined Radio.

Copyright (c) 2022-2024 Marat Fayzullin <luarvique@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ANDRAS RETZLER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "module.hpp"

namespace Csdr {

    template <typename T>
    class CwDecoder: public Module<T, unsigned char> {
        public:
            CwDecoder(unsigned int sampleRate, bool showCw=false, unsigned int inst=0);

            bool canProcess() override;
            void process() override;

            void reset();
            int getWPM();

        private:
            // Configurable input parameters
            unsigned int sampleRate;      // Input sampling rate (Hz)
            unsigned int nbTime;          // Noise blanker time (ms)
            unsigned int quTime;          // Quantization step (ms)
            unsigned int dbgTime;         // Debug printout time (ms)
            bool showCw;                  // TRUE: show dits/dahs
            unsigned int inst;

            // Time counting
            unsigned long curSeconds = 0; // Current time in seconds
            unsigned int  curSamples = 0; // Sample count since last second mark
            unsigned int  quStep;         // Quantization step (samples)
            double attack;                // Attack factor for MagL/MagH
            double decay;                 // Decay factor for MagL/MagH

            // Input signal characteristics
            double magL = 0.5;            // Minimal observed magnitude
            double magH = 0.5;            // Maximal observed magnitude
            bool realState0 = false;      // Last unfiltered signal state (0/1)
            bool filtState0 = false;      // Last filtered signal state (0/1)

            // HIGH / LOW timing
            unsigned long lastStartT = 0; // Time of the last signal change (ms)
            unsigned long startTimeH = 0; // Time HIGH signal started (ms)
            unsigned long startTimeL = 0; // Time LOW signal started (ms)

            // DIT / DAH / BREAK timing
            double avgDitT = 50;          // Average DIT signal duration (ms)
            double avgDahT = 100;         // Average DAH signal duration (ms)
            double avgBrkT = 50;          // Average BREAK duration (ms)

            // Current CW code
            unsigned int code = 1;        // Currently accumulated CW code or 1
            unsigned int wpm  = 0;        // Current CW speed (in wpm)

            // Code to character conversion table
            static const char cwTable[];

            // Debugging data
            unsigned long lastDebugT = 0; // Time of the last debug printout (ms)

            // Convert input sample into signal level
            inline double sample2level(T input);

            // Get current time in milliseconds
            unsigned long msecs()
            { return(1000*curSeconds + 1000*curSamples/sampleRate); }

            // Get number of samples in given number of milliseconds
            unsigned int ms2smp(unsigned int msec)
            { return(sampleRate * msec / 1000); }

            // Convert CW code to a character
            char cw2char(unsigned int data)
            { return(data<256? cwTable[data] : '_'); }

            // Process a quantum of input data
            void processInternal(bool newState);

            // Print debug information
            void printDebug();

            // Print given string
            void printString(const char *buf);
    };
}
