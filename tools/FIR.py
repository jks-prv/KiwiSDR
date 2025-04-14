# RX-888 Team, support@rx-888.com, Licensed under CC-BY-SA 4.0
# github.com/RaspSDR/server/blob/master/tools/FIR.py

import numpy as np
from scipy.signal import firls, freqz
import matplotlib.pyplot as plt

def mk(sr, filter_type):
    # Make NFM deemphasis filter. Parameters: samplerate, filter length, frequency to normalize at (filter gain will be 0dB at this frequency)
    tapnum = 79
    attn = -80
    norm_freq = 400

    def normalize_at_freq(vect, freq, sr):
        t = np.arange(0, len(vect)) / sr
        return vect / np.dot(vect, np.sin(2 * np.pi * freq * t))

    if filter_type == 0:
        # -20 dB / dec (-6.02 dB / oct)
        # rolloff below 400 Hz
        print("NFM -LF")
        if sr == 12000:
            freqvect = [0, 200, 200, 400, 400, 600, 600, 800, 800, 1200, 1200, 1600, 1600, 3200, 3200, 4000, 4000, 5000, 5000, 6000]
            loss = [-80, -80, -80, 0, 0, -3.5, -3.5, -6, -6, -9.54, -9.54, -12, -12, -18, -18, -20, -20, -22, -22, -23]
        else:
            freqvect = [0, 200, 200, 400, 400, 600, 600, 800, 800, 1200, 1200, 1600, 1600, 3200, 3200, 4000, 4000, 5000, 5000, 6000, 6000, sr / 2]
            loss = [-80, -80, -80, 0, 0, -3.5, -3.5, -6, -6, -9.54, -9.54, -12, -12, -18, -18, -20, -20, -22, -22, -23, -23, -28]
        resp = 10 ** (np.array(loss) / 20.0)
        coeffs = firls(tapnum, np.array(freqvect) / (sr / 2), resp)
    elif filter_type == 1:
        # -20 dB / dec (-6.02 dB / oct)
        # flat to 0 Hz
        print("NFM +LF")
        if sr == 12000:
            freqvect = [0, 400, 400, 600, 600, 800, 800, 1200, 1200, 1600, 1600, 3200, 3200, 4000, 4000, 5000, 5000, 6000]
            loss = [0, 0, 0, -3.5, -3.5, -6, -6, -9.54, -9.54, -12, -12, -18, -18, -20, -20, -22, -22, -23]
        else:
            freqvect = [0, 400, 400, 600, 600, 800, 800, 1200, 1200, 1600, 1600, 3200, 3200, 4000, 4000, 5000, 5000, 6000, 6000, sr / 2]
            loss = [0, 0, 0, -3.5, -3.5, -6, -6, -9.54, -9.54, -12, -12, -18, -18, -20, -20, -22, -22, -23, -23, -28]
        resp = 10 ** (np.array(loss) / 20.0)
        coeffs = firls(tapnum, np.array(freqvect) / (sr / 2), resp)
    elif filter_type == 2:
        print("AM/SSB 75 uS")
        if sr == 12000:
            freqvect = [0, 1000, 1000, 2000, 2000, 3000, 3000, 4000, 4000, 5000, 5000, sr / 2]
            loss = [0, -0.8, -0.8, -2.5, -2.5, -4.3, -4.3, -5.7, -5.7, -7.0, -7.0, -7.8]
        else:
            freqvect = [0, 1000, 1000, 2000, 2000, 3000, 3000, 4000, 4000, 5000, 5000, 6000, 6000, 7000, 7000, 8000, 8000, 9000, 9000, sr / 2]
            loss = [0, -0.8, -0.8, -2.5, -2.5, -4.3, -4.3, -5.7, -5.7, -7.0, -7.0, -7.8, -7.8, -8.6, -8.6, -9.2, -9.2, -9.6, -9.6, -10.0]
        resp = 10 ** (np.array(loss) / 20.0)
        coeffs = firls(tapnum, np.array(freqvect) / (sr / 2), resp)
    elif filter_type == 3:
        print("AM/SSB 50 uS")
        if sr == 12000:
            freqvect = [0, 1000, 1000, 2000, 2000, 3000, 3000, 4000, 4000, 5000, 5000, sr / 2]
            loss = [0, -0.4, -0.4, -1.3, -1.3, -2.4, -2.4, -3.5, -3.5, -4.5, -4.5, -5.4]
        else:
            freqvect = [0, 1000, 1000, 2000, 2000, 3000, 3000, 4000, 4000, 5000, 5000, 6000, 6000, 7000, 7000, 8000, 8000, 9000, 9000, sr / 2]
            loss = [0, -0.4, -0.4, -1.3, -1.3, -2.4, -2.4, -3.5, -3.5, -4.5, -4.5, -5.4, -5.4, -6.1, -6.1, -6.7, -6.7, -7.2, -7.2, -7.6]
        resp = 10 ** (np.array(loss) / 20.0)
        coeffs = firls(tapnum, np.array(freqvect) / (sr / 2), resp)
    elif filter_type == 4:
        print("-20 dB test")
        freqvect = [0, 1000, 1000, 2000, 2000, 3000, 3000, 4000, 4000, sr / 2]
        mag_resp_dB = [0, 0, 0, -20, -20, -20, -20, 0, 0, 0]
        mag_resp = 10 ** (np.array(mag_resp_dB) / 20.0)
        coeffs = firls(tapnum, np.array(freqvect) / (sr / 2), mag_resp)
    else:
        print("fir2")
        freqvect = [0, 400, 400, 4000, 4000, sr / 2]
        loss = [0, 0, 0, attn, attn, attn]
        resp = 10 ** (np.array(loss) / 20.0)
        coeffs = firls(tapnum, np.array(freqvect) / (sr / 2), resp)
        print(f"size coeffs fir2: {coeffs.shape}")

    coeffs = normalize_at_freq(coeffs, norm_freq, sr)
    w, h = freqz(coeffs)
    # plt.plot(w / np.pi * (sr / 2), 20 * np.log10(abs(h)))
    # plt.title("Frequency Response")
    # plt.xlabel("Frequency (Hz)")
    # plt.ylabel("Magnitude (dB)")
    # plt.grid()
    # plt.show()

    print(f"size coeffs: {coeffs.shape}")
    print(", ".join(f"{c:.9f}" for c in coeffs))

#mk(12000, 0)
#mk(12000, 1)
#mk(12000, 2)
#mk(12000, 3)

#mk(24000, 0)
#mk(24000, 1)
#mk(24000, 2)
#mk(24000, 3)

mk(48000, 0)
mk(48000, 1)
mk(48000, 2)
mk(48000, 3)
