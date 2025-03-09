#include "csdr/ringbuffer.hpp"
#include "csdr/cw.hpp"
#include "fftw3.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define MAX_SCALES   (16)
#define MAX_CHANNELS (MAX_SND_RATE / 2 / 100)
#undef  MAX_INPUT
#define MAX_INPUT    (MAX_CHANNELS * 2)
#define AVG_SECONDS  (3)
#define NEIGH_WEIGHT (0.5)
#define THRES_WEIGHT (6.0)

#include <functional>
typedef std::function<void(int, char, int)> OutputCallback;

class CwSkimmer {
public:
    CwSkimmer(unsigned int srate)
        : sampleRate(srate) {
        printf("CwSkimmer(%d)\n", sampleRate);
        max_channels = sampleRate / 2 / 100;
        max_input = max_channels * 2;

        // initialize fftw
        fft = fftwf_plan_dft_r2c_1d(max_input, fftIn, fftOut, FFTW_ESTIMATE);

        // Allocate CSDR object storage
        in = new Csdr::Ringbuffer<float>*[MAX_CHANNELS];
        inReader = new Csdr::RingbufferReader<float>*[MAX_CHANNELS];
        
        out = new Csdr::Ringbuffer<unsigned char>*[MAX_CHANNELS];
        outReader = new Csdr::RingbufferReader<unsigned char>*[MAX_CHANNELS];
        
        cwDecoder = new Csdr::CwDecoder<float>*[MAX_CHANNELS];
        outState = new unsigned int[MAX_CHANNELS];

        // Create and connect CSDR objects, clear output state
        for (int j = 0; j < max_channels; ++j) {
            in[j] = new Csdr::Ringbuffer<float>(sampleRate);
            inReader[j] = new Csdr::RingbufferReader<float>(in[j]);
            
            out[j] = new Csdr::Ringbuffer<unsigned char>(printChars * 4);
            outReader[j] = new Csdr::RingbufferReader<unsigned char>(out[j]);
            
            cwDecoder[j] = new Csdr::CwDecoder<float>(sampleRate, false, j);
            cwDecoder[j]->setReader(inReader[j]);
            cwDecoder[j]->setWriter(out[j]);
            outState[j] = ' ';
        }

        remains = 0;
        avgPower = 4.0;
    }

    ~CwSkimmer() {
        // Release FFTW3 resources
        fftwf_destroy_plan(fft);

        // Release CSDR resources
        for (int j = 0; j < max_channels; ++j) {
            delete outReader[j];
            delete out[j];
            delete cwDecoder[j];
            delete inReader[j];
            delete in[j];
        }

        // Release CSDR object storage
        delete[] in;
        delete[] inReader;
        delete[] out;
        delete[] outReader;
        delete[] cwDecoder;
        delete[] outState;
    }

    void flush() {
        // Final printout
        for (int i = 0; i < max_channels; i++) {
            printOutput(i, i * sampleRate / 2 / max_channels, 1);
            outState[i] = ' ';
            cwDecoder[i]->reset();
        }
        remains = 0;
        avgPower = 4.0;
    }

    void SetParams(unsigned int pwr_calc, unsigned int filter_neighbors) {
        this->pwr_calc = pwr_calc;
        this->filter_neighbors = filter_neighbors;
    }
    
    void SetCallback(OutputCallback callback) {
        this->callback = callback;

        for (int i = 0; i < max_channels; i++) {
            outState[i] = ' ';
            cwDecoder[i]->reset();
        }
        
        remains = 0;
        avgPower = 4.0;
    }

    void AddSamples(float* sample, size_t count) {
        while (count > 0) {
            size_t avail = max_input - remains;
            if (count > avail) {
                // more samples than we can fit in the buffer
                memcpy(dataBuf + remains, sample, (avail) * sizeof(float));
                count -= avail;
                sample += avail;
                process();
                remains = 0;
            }
            else {
                // we can fit the samples in the buffer
                memcpy(dataBuf + remains, sample, count * sizeof(float));

                remains += count;
                if (remains == 0) {
                    process();
                    remains = 0;
                }
                break;
            }
        }
    }

    void AddSamples(int16_t* sample, size_t count) {
        while (count > 0) {
            size_t avail = max_input - remains;
            if (count > avail) {
                // more samples than we can fit in the buffer
                for (size_t i = 0; i < avail; i++) {
                    dataBuf[remains + i] = sample[i] / 32768.0;
                }
                count -= avail;
                sample += avail;
                process();
                remains = 0;
            }
            else {
                // we can fit the samples in the buffer
                for (size_t i = 0; i < count; i++) {
                    dataBuf[remains + i] = sample[i] / 32768.0;
                }

                remains += count;
                if (remains == 0) {
                    process();
                    remains = 0;
                }
                break;
            }
        }
    }

private:
    unsigned int sampleRate, max_channels, max_input;
    float avgPower;
    size_t remains;
    unsigned int filter_neighbors;
    
    #define PWR_CALC_AVG_RATIO  0   // Divide each bucket by average value
    #define PWR_CALC_AVG_BOTTOM 1   // Subtract average value from each bucket
    #define PWR_CALC_THRESHOLD  2   // Convert each bucket to 0.0/1.0 values
    unsigned int pwr_calc;

    unsigned int printChars = 8;     // Number of characters to print at once

    fftwf_complex fftOut[MAX_INPUT];
    short dataIn[MAX_INPUT];
    float dataBuf[MAX_INPUT];
    float fftIn[MAX_INPUT];
    fftwf_plan fft;

    Csdr::Ringbuffer<float>** in;
    Csdr::RingbufferReader<float>** inReader;
    Csdr::Ringbuffer<unsigned char>** out;
    Csdr::RingbufferReader<unsigned char>** outReader;
    Csdr::CwDecoder<float>** cwDecoder;
    unsigned int* outState;

    OutputCallback callback;

    // Hamming window function
    float hamming(unsigned int x, unsigned int size) {
        return (0.54 - 0.46 * cosf((2.0 * M_PI * x) / (size - 1)));
    }

    // when there is enough samples
    void process() {
        float maxPower, accPower;
        // Apply Hamming window
        const float hk = 2.0 * M_PI / (max_input - 1);
        for (int j = 0; j < max_input; ++j)
            fftIn[j] = dataBuf[j] * (0.54 - 0.46 * cosf(j * hk));

        // Compute FFT
        fftwf_execute(fft);

        // Go to magnitudes
        for (int j = 0; j < max_input / 2; ++j)
            fftOut[j][0] = fftOut[j][1] = sqrtf(fftOut[j][0] * fftOut[j][0] + fftOut[j][1] * fftOut[j][1]);

        // Filter out spurs
        if (filter_neighbors) {
            fftOut[max_input / 2 - 1][0] = fmaxf(0.0, fftOut[max_input / 2 - 1][1] - NEIGH_WEIGHT * fftOut[max_input / 2 - 2][1]);
            fftOut[0][0] = fmaxf(0.0, fftOut[0][1] - NEIGH_WEIGHT * fftOut[1][1]);
            for (int j = 1; j < max_input / 2 - 1; ++j)
                fftOut[j][0] = fmaxf(0.0, fftOut[j][1] - 0.5 * NEIGH_WEIGHT * (fftOut[j - 1][1] + fftOut[j + 1][1]));
        }

        struct
        {
            float power;
            int count;
        } scales[MAX_SCALES];

        // Sort buckets into scales
        maxPower = 0.0;
        memset(scales, 0, sizeof(scales));
        for (int j = 0; j < max_input / 2; ++j) {
            float v = fftOut[j][0];
            int scale = floorf(logf(v));
            scale = scale < 0 ? 0 : scale + 1 >= MAX_SCALES ? MAX_SCALES - 1
                                                            : scale + 1;
            maxPower = fmaxf(maxPower, v);
            scales[scale].power += v;
            scales[scale].count++;
        }

        // Find most populated scales and use them for ground power
        int n = 0;
        accPower = 0.0;
        for (int i = 0; i < MAX_SCALES - 1; ++i) {
            // Look for the next most populated scale
            int k, j;
            for (k = i, j = i + 1; j < MAX_SCALES; ++j)
                if (scales[j].count > scales[k].count) k = j;
            // If found, swap with current one
            if (k != i) {
                float v = scales[k].power;
                j = scales[k].count;
                scales[k] = scales[i];
                scales[i].power = v;
                scales[i].count = j;
            }
            // Keep track of the total number of buckets
            accPower += scales[i].power;
            n += scales[i].count;
            // Stop when we collect 1/2 of all buckets
            if (n >= max_input / 2 / 2) break;
        }

        // fprintf(stderr, "accPower = %f (%d buckets, %d%%)\n", accPower/n, i+1, 100*n*2/max_input);

        // Maintain rolling average over AVG_SECONDS
        accPower /= n;
        avgPower += (accPower - avgPower) * max_input / sampleRate / AVG_SECONDS;

        // Decode by channel
        int i, j, k;
        for (j = i = k = n = 0, accPower = 0.0; j < max_input / 2; ++j, ++n) {
            float power = fftOut[j][0];

            // If accumulated enough FFT buckets for a channel...
            if (k >= max_input / 2) {
                switch (pwr_calc) {
                
                    case PWR_CALC_AVG_RATIO:
                        // Divide channel signal by the average power
                        accPower = fmaxf(1.0, accPower / fmaxf(avgPower, 0.000001));
                        break;

                    case PWR_CALC_AVG_BOTTOM:
                        // Subtract average power from the channel signal
                        accPower = fmaxf(0.0, accPower - avgPower);
                        break;

                    case PWR_CALC_THRESHOLD:
                        // Convert channel signal to 1/0 values based on threshold
                        accPower = accPower >= avgPower * THRES_WEIGHT ? 1.0 : 0.0;
                        break;
                }

                // If CW input buffer can accept samples...
                if (in[i]->writeable() >= max_input) {
                    // Fill input buffer with computed signal power
                    float* dst = in[i]->getWritePointer();
                    for (int m = 0; m < max_input; ++m) dst[m] = accPower;
                    in[i]->advance(max_input);

                    // Process input for the current channel
                    while (cwDecoder[i]->canProcess()) {
                        NextTask("cws1");
                        cwDecoder[i]->process();
                        NextTask("cws2");
                    }

                    // Print output
                    printOutput(i, i * sampleRate / 2 / max_channels, printChars);
                }

                // Start on a new channel
                accPower = 0.0;
                k -= max_input / 2;
                i += 1;
                n = 0;
            }

            // Maximize channel signal power
            accPower = fmaxf(accPower, power);
            k += max_channels;
        }
    }

    // Print output from ith decoder
    void
    printOutput(int i, unsigned int freq, unsigned int printChars) {
        // Must have a minimum of printChars
        size_t n = outReader[i]->available();
        if (n < printChars) return;
        int wpm = cwDecoder[i]->getWPM();

        // Print characters
        unsigned char* p = outReader[i]->getReadPointer();
        for (size_t j = 0; j < n; ++j) {
            switch (outState[i] & 0xFF) {
            case '\0':
                // Print character
                callback(freq, p[j], wpm);
                // Once we encounter a space, wait for stray characters
                if (p[j] == ' ') outState[i] = p[j];
                break;
            case ' ':
                // If possible error, save it in state, else print and reset state
                if (strchr("TEI ", p[j]))
                    outState[i] = p[j];
                else {
                    callback(freq, p[j], wpm);
                    outState[i] = '\0';
                }
                break;
            default:
                // If likely error, skip it, else print and reset state
                if (strchr("TEI ", p[j]))
                    outState[i] = (outState[i] << 8) | p[j];
                else {
                    for (int k = 24; k >= 0; k -= 8)
                        if ((outState[i] >> k) & 0xFF)
                            callback(freq, (outState[i] >> k) & 0xFF, wpm);
                    callback(freq, p[j], wpm);
                    outState[i] = '\0';
                }
                break;
            }
        }

        // Done printing
        outReader[i]->advance(n);
    }
};
