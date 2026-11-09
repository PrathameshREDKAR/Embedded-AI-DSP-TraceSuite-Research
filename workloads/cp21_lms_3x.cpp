#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstring>

extern "C" __declspec(dllexport) __declspec(noinline)
void roi_begin()
{
    volatile int marker = 0;
    (void)marker;
}

extern "C" __declspec(dllexport) __declspec(noinline)
void roi_end()
{
    volatile int marker = 0;
    (void)marker;
}

static const int SIGNAL_LENGTH = 8192;
static const int ADAPTIVE_TAPS = 64;
static const int UNKNOWN_TAPS = 16;
static const int LMS_RUNS = 3;
static const double MU = 0.01;

static double x[SIGNAL_LENGTH];
static double d[SIGNAL_LENGTH];

static const double h[UNKNOWN_TAPS] = {
    0.35, -0.22, 0.18, 0.11,
    -0.09, 0.07, 0.05, -0.04,
    0.03, -0.025, 0.02, -0.015,
    0.012, -0.01, 0.008, -0.006
};

int main()
{
    uint32_t state = 0x12345678u;

    for (int n = 0; n < SIGNAL_LENGTH; ++n) {
        state = 1664525u * state + 1013904223u;
        const uint32_t top24 = state >> 8;
        x[n] = 2.0 * (static_cast<double>(top24) / 16777215.0) - 1.0;
    }

    for (int n = 0; n < SIGNAL_LENGTH; ++n) {
        double sum = 0.0;
        const int kmax = (n < UNKNOWN_TAPS - 1) ? n : (UNKNOWN_TAPS - 1);
        for (int k = 0; k <= kmax; ++k) {
            sum += h[k] * x[n - k];
        }
        d[n] = sum;
    }

    bool finite_ok = true;
    for (int n = 0; n < SIGNAL_LENGTH; ++n) {
        if (!std::isfinite(x[n]) || !std::isfinite(d[n])) {
            finite_ok = false;
        }
    }

    double w[ADAPTIVE_TAPS];
    double initial_mse = 0.0;
    double final_mse = 0.0;

    roi_begin();

    for (int run = 0; run < LMS_RUNS; ++run) {
        for (int k = 0; k < ADAPTIVE_TAPS; ++k) {
            w[k] = 0.0;
        }

        double initial_sse = 0.0;
        double final_sse = 0.0;

        for (int n = 63; n < SIGNAL_LENGTH; ++n) {
            double y = 0.0;

            for (int k = 0; k < ADAPTIVE_TAPS; ++k) {
                y += w[k] * x[n - k];
            }

            const double e = d[n] - y;

            if (!std::isfinite(y) || !std::isfinite(e)) {
                finite_ok = false;
            }

            if (n >= 63 && n <= 574) {
                initial_sse += e * e;
            }

            if (n >= 7680 && n <= 8191) {
                final_sse += e * e;
            }

            for (int k = 0; k < ADAPTIVE_TAPS; ++k) {
                w[k] += MU * e * x[n - k];
            }
        }

        initial_mse = initial_sse / 512.0;
        final_mse = final_sse / 512.0;
    }

    roi_end();

    double weight_sse = 0.0;

    for (int k = 0; k < ADAPTIVE_TAPS; ++k) {
        const double reference = (k < UNKNOWN_TAPS) ? h[k] : 0.0;

        if (!std::isfinite(w[k])) {
            finite_ok = false;
        }

        const double diff = w[k] - reference;
        weight_sse += diff * diff;
    }

    const double weight_rmse = std::sqrt(weight_sse / static_cast<double>(ADAPTIVE_TAPS));

    if (!std::isfinite(initial_mse) ||
        !std::isfinite(final_mse) ||
        !std::isfinite(weight_rmse)) {
        finite_ok = false;
    }

    const bool oracle_a = finite_ok && (final_mse < initial_mse);
    const bool oracle_b = finite_ok && (final_mse <= 1.0e-2);
    const bool oracle_c = finite_ok && (weight_rmse <= 5.0e-2);

    std::printf("LMS_RUNS=%d\n", LMS_RUNS);
    std::printf("SIGNAL_LENGTH=%d\n", SIGNAL_LENGTH);
    std::printf("ADAPTIVE_TAPS=%d\n", ADAPTIVE_TAPS);
    std::printf("UNKNOWN_SYSTEM_TAPS=%d\n", UNKNOWN_TAPS);
    std::printf("MU=%.17g\n", MU);
    std::printf("INITIAL_WINDOW_MSE=%.17g\n", initial_mse);
    std::printf("FINAL_WINDOW_MSE=%.17g\n", final_mse);
    std::printf("WEIGHT_RMSE=%.17g\n", weight_rmse);
    std::printf("FINAL_W0=%.17g\n", w[0]);
    std::printf("FINAL_W1=%.17g\n", w[1]);
    std::printf("FINAL_W15=%.17g\n", w[15]);
    std::printf("FINAL_W16=%.17g\n", w[16]);
    std::printf("FINITE_OK=%d\n", finite_ok ? 1 : 0);
    std::printf("ORACLE_A_FINAL_LT_INITIAL=%s\n", oracle_a ? "PASS" : "FAIL");
    std::printf("ORACLE_B_FINAL_MSE_LE_1E-2=%s\n", oracle_b ? "PASS" : "FAIL");
    std::printf("ORACLE_C_WEIGHT_RMSE_LE_5E-2=%s\n", oracle_c ? "PASS" : "FAIL");

    if (!(oracle_a && oracle_b && oracle_c)) {
        std::printf("LMS_1X_NATIVE_FAIL\n");
        return 2;
    }

    std::printf("LMS_1X_NATIVE_OK\n");
    return 0;
}