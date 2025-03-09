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

#include "cw.hpp"
#include "str.h"
#include <cmath>
#include <cstring>
#include <cstdio>

using namespace Csdr;

template <typename T>
const char CwDecoder<T>::cwTable[] =
    "__TEMNAIOGKDWRUS" // 00000000
    "__QZYCXBJP_L_FVH"
    "09_8_<_7_(___/-6" // <AR>
    "1______&2___3_45"
    "_______:____,___" // 01000000
    "__)_!;________-_"
    "_'___@____._____"
    "___?______{_____" // <SK>
    "________________" // 10000000
    "________________"
    "________________"
    "________________"
    "________________" // 11000000
    "________________"
    "________________"
    "______$_________";

template <typename T>
CwDecoder<T>::CwDecoder(unsigned int sampleRate, bool showCw, unsigned int inst)
: sampleRate(sampleRate),
  quTime(5),      // Quantization step (ms)
  nbTime(20),     // Noise blanking width (ms)
  dbgTime(0),     // Debug printout period (ms)
  showCw(showCw), // TRUE: print DITs/DAHs
  inst(inst)      // instance
{
    // Minimal number of samples to process, attack and decay factors
    quStep = quTime * sampleRate / 1000;
    attack = (double)quTime / 50.0;
    decay  = (double)quTime / 5000.0;
}

template <typename T>
void CwDecoder<T>::reset() {
    std::lock_guard<std::mutex> lock(this->processMutex);

    // Input signal characteristics
    realState0 = false; // Last unfiltered signal state (0/1)
    filtState0 = false; // Last filtered signal state (0/1)
    magL = 0.5;     // Minimal observed magnitude
    magH = 0.5;     // Maximal observed magnitude

    // HIGH / LOW timing
    lastStartT = 0; // Time of the last signal change (ms)
    startTimeH = 0; // Time HIGH signal started (ms)
    startTimeL = 0; // Time LOW signal started (ms)

    // DIT / DAH / BREAK timing
    avgDitT = 50;   // Average DIT signal duration (ms)
    avgDahT = 100;  // Average DAH signal duration (ms)
    avgBrkT = 50;   // Average BREAK duration (ms)

    // Current CW code
    code = 1;       // Currently accumulated CW code or 1
    wpm  = 0;       // Current CW speed (in wpm)
}

template <typename T>
int CwDecoder<T>::getWPM() {
    return wpm;
}

template <typename T>
bool CwDecoder<T>::canProcess() {
    std::lock_guard<std::mutex> lock(this->processMutex);
    return this->reader->available() >= quStep;
}

template <typename T>
void CwDecoder<T>::process() {
    std::lock_guard<std::mutex> lock(this->processMutex);

    // Must have at least quStep samples
    if(this->reader->available()<quStep) return;

    const T *data = this->reader->getReadPointer();
    double range = magH - magL;
    double magnitude = 0.0;

    // Compute overall magnitude
    for(unsigned int i=0 ; i<quStep ; ++i)
        magnitude += sample2level(data[i]);

    this->reader->advance(quStep);
    magnitude /= quStep;

    // Compute current state based on the magnitude
    bool realState =
        magnitude>(magL+range*0.7)? 1 :
        magnitude<(magL+range*0.5)? 0 :
        realState0;

    // Keep track of minimal/maximal magnitude
    magL += magnitude<magL? (magnitude-magL)*attack :  range*decay;
    magH += magnitude>magH? (magnitude-magH)*attack : -range*decay;

    // Process input
    processInternal(realState);

    // Update time
    curSamples += quStep;
    if(curSamples>=sampleRate)
    {
        unsigned int secs = curSamples/sampleRate;
        curSeconds += secs;
        curSamples -= secs*sampleRate;
    }
}

template <typename T>
void CwDecoder<T>::processInternal(bool newState) {
    unsigned long millis = msecs();
    unsigned int i, j;
    double duration;

    // Filter out jitter based on nbTime
    if(newState!=realState0) lastStartT = millis;
    bool filtState = (millis-lastStartT)>nbTime? newState : filtState0;

    // If signal state changed...
    if(filtState!=filtState0)
    {
        if(filtState)
        {
            // Ending a LOW state...

            // Compute LOW duration
            startTimeH = millis;
            duration   = millis - startTimeL;

            // If we have got some DITs or DAHs and there is a BREAK...
            if((code>1) && (duration>=2.5*avgBrkT))
            {
                // Print character
                *(this->writer->getWritePointer()) = cw2char(code);
                this->writer->advance(1);

                // If a word BREAK, print a space...
                if(duration>=5.0*avgBrkT)
                {
                    *(this->writer->getWritePointer()) = ' ';
                    this->writer->advance(1);
                }

                // Start new character
                code = 1;
            }

            // Keep track of the average small BREAK duration
            if((duration>20.0) && (duration<1.5*avgDitT) && (duration>0.6*avgDitT))
                avgBrkT += (duration - avgBrkT)/4.0;
        }
        else
        {
            // Ending a HIGH state...

            // Compute HIGH duration
            startTimeL = millis;
            duration   = millis - startTimeH;

            double midT = (avgDitT + avgDahT)/2.0;

            // Filter out false DITs
            if((duration<=midT) && (duration>0.5*avgDitT))
            {
                // Add a DIT to the code
                code = (code<<1) | 1;

                // Print a DIT
                if(showCw)
                {
                    *(this->writer->getWritePointer()) = '.';
                    this->writer->advance(1);
                }
            }
            else if((duration>midT) && (duration<3.0*avgDahT))
            {
                // Add a DAH to the code
                code = (code<<1) | 0;

                // Try computing WPM
                wpm = (wpm + (int)(3600.0/duration))/2;

                // Print a DAH
                if(showCw)
                {
                    *(this->writer->getWritePointer()) = '-';
                    this->writer->advance(1);
                }
            }

            // Keep track of the average DIT duration
            if((duration>20.0) && (duration<0.4*avgDahT))
                avgDitT += (duration - avgDitT)/4.0;

            // Keep track of the average DAH duration
            if((duration<500.0) && (duration>2.5*avgDitT))
                avgDahT += (duration - avgDahT)/4.0;
        }
    }

    // If long silence, print the last buffered character
    if((code>1) && !filtState && ((millis-startTimeL)>5.0*avgBrkT))
    {
        // Print character
        *(this->writer->getWritePointer()) = cw2char(code);
        this->writer->advance(1);

        // Print word break
        *(this->writer->getWritePointer()) = ' ';
        this->writer->advance(1);

        // Start new character
        code = 1;
    }

    // Periodically print debug information, if enabled
    if(dbgTime && (millis-lastDebugT >= dbgTime))
    {
        lastDebugT = millis;
        printDebug();
    }

    // Update state
    realState0 = newState;
    filtState0 = filtState;
}

template <typename T>
void CwDecoder<T>::printDebug()
{
    char buf[256];

    // Create complete string to print
    kiwi_snprintf_buf(buf, "[%d-%d .%d -%d _%dms WPM%d]\n", (int)magL, (int)magH, (int)avgDitT, (int)avgDahT, (int)avgBrkT, wpm);

    // Print
    printString(buf);
}

template <typename T>
void CwDecoder<T>::printString(const char *buf)
{
    unsigned int l = strlen(buf);

    // If there is enough output buffer available...
    if(this->writer->writeable()>=l)
    {
        // Write data then advance pointer
        memcpy(this->writer->getWritePointer(), buf, l);
        this->writer->advance(l);
    }
}

template <>
inline double CwDecoder<complex<float>>::sample2level(complex<float> input)
{
    return sqrt((input.i() * input.i()) + (input.q() * input.q()));
}

template<>
inline double CwDecoder<float>::sample2level(float input)
{
    return fabs(input);
}

namespace Csdr {
    template class CwDecoder<complex<float>>;
    template class CwDecoder<float>;
}
