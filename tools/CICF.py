# RX-888 Team, support@rx-888.com, Licensed under CC-BY-SA 4.0
# github.com/RaspSDR/server/blob/master/tools/CICF.py

import numpy as np
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt

lower_bounds = [-10,  0]   # p1 ∈ [-10, 10], p2 ∈ [0, 100]
upper_bounds = [  0, 100]

def fit_two_stage_cic_compensation(
    N1, R1, M1,
    N2, R2, M2,
    num_points=300,
    initial_guess=(-3.0, 30.0),
    plot=False
):
    """
    Fitting two levels with different parameters CIC Series compensation filter parameters p1, p2，
    Make
        H_model(f) = H_target(f) + p1 * exp(p2*(f-0.5))
    in
        H_target(f) = [sinc(R1*M1*f)]^{-N1} * [sinc(R2*M2*f)]^{-N2}

    parameter:
      N1, R1, M1: the number of stages, decimation rate, and differential delay of the first CIC
      N2, R2, M2: the number of levels, decimation rate, and differential delay of the second-level CIC
      num_points: the number of frequency points used in the fitting
      initial_guess: the initial guess of (p1, p2)
      plot: whether to draw the Result: ratio graph

    return:
      p_opt: fitted [p1, p2]
      p_cov: covariance matrix
    """
    # Avoid division by zero when f=0
    eps = 1e-8
    f = np.linspace(eps, 0.5, num_points)

    # Normalization sinc：np.sinc(x) = sin(pi x)/(pi x)
    def sinc(x):
        return np.sinc(x)

    # Target compensation response: the inverse of the two cascades
    if N2 == 0:
        H_target = (sinc(R1 * M1 * f)**(-N1))
    else:
        H_target = (sinc(R1 * M1 * f)**(-N1)) * (sinc(R2 * M2 * f)**(-N2))

    # Model function: main compensation + exponential term
    def H_model(f, p1, p2):
        return H_target + p1 * np.exp(p2 * (f - 0.5))

    # Fitting
    p_opt, p_cov = curve_fit(H_model, f, H_target, p0=initial_guess)

    if plot:
        plt.figure()
        plt.plot(f, H_target,    label="Target $H_{target}(f)$")
        plt.plot(f, H_model(f, *p_opt), '--', label="Fitting $H_{model}(f)$")
        plt.xlabel("Normalized frequency $f$")
        plt.ylabel("Amplitude")
        plt.title(
            f"Two-stage CIC series compensation fitting\n"
            f"Stage1: N={N1}, R={R1}, M={M1} | "
            f"Stage2: N={N2}, R={R2}, M={M2}"
        )
        plt.legend()
        plt.grid(True)
        plt.show()

    return p_opt, p_cov

if __name__ == "__main__":

    N1, R1, M1 = 5, 8192, 1

    p_opt, p_cov = fit_two_stage_cic_compensation(
        N1, R1, M1,
        0, 0 ,0
    )
    print(f"// WF CICF")
    print(f"\t{p_opt[0]:.4f}, {p_opt[1]:.4f},")

    N1, R1, M1 = 3, 256, 1
    N2, R2, M2 = 5, 40, 1

    for R2 in range(10, 41):
        p_opt, p_cov = fit_two_stage_cic_compensation(
            N1, R1, M1,
            N2, R2, M2,
        )
        SampleRate = 122.88 * 1000 / (R1 * R2)
        print(f"// R2={R2} SampleRate: {SampleRate:.2f} kHz")
        print(f"\t{p_opt[0]:.4f}, {p_opt[1]:.4f},")

    # 24K
    # p1 = -0.0046
    # p2 = 35.0944
    # 12K
    # p1 = -0.6364
    # p2 = 46.6379
