#include <cstdint>
#include <cstdio>
#include <cmath>
#include <algorithm>

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

static constexpr int N = 16384;
static constexpr int TAPS = 128;
static constexpr int ECHO_TAPS = 32;
static constexpr int NLMS_RUNS = 1;
static constexpr double MU = 0.5;
static constexpr double EPSILON = 1.0e-6;
static constexpr int FIRST = 127;
static constexpr int INITIAL_LAST = 638;
static constexpr int FINAL_FIRST = 15872;
static constexpr int WINDOW = 512;

static double x[N];
static double d[N];
static double h[ECHO_TAPS];
static double w[TAPS];

struct Metrics {
    double initial_mse;
    double final_mse;
};

static bool finite_array(const double* a, int count)
{
    for (int i = 0; i < count; ++i) {
        if (!std::isfinite(a[i])) return false;
    }
    return true;
}

static void generate_input()
{
    uint32_t state = 0x12345678u;

    for (int n = 0; n < N; ++n) {
        state = 1664525u * state + 1013904223u;
        const uint32_t top24 = state >> 8;
        x[n] = 2.0 * (static_cast<double>(top24) /
                      16777215.0) - 1.0;
    }
}

static void generate_echo()
{
    for (int k = 0; k < ECHO_TAPS; ++k) {
        const double t = static_cast<double>(k);
        h[k] = 0.7 * std::exp(-0.12 * t) *
               std::cos(0.37 * t);
    }

    for (int n = 0; n < N; ++n) {
        double sum = 0.0;

        const int count = std::min(n + 1, ECHO_TAPS);

        for (int k = 0; k < count; ++k) {
            sum += h[k] * x[n - k];
        }

        d[n] = sum;
    }
}

static Metrics production_nlms()
{
    double initial_sse = 0.0;
    double final_sse = 0.0;

    for (int n = FIRST; n < N; ++n) {
        double output = 0.0;
        double energy = EPSILON;

        for (int k = 0; k < TAPS; ++k) {
            const double sample = x[n - k];
            output += w[k] * sample;
            energy += sample * sample;
        }

        const double error = d[n] - output;

        if (n <= INITIAL_LAST) {
            initial_sse += error * error;
        }

        if (n >= FINAL_FIRST) {
            final_sse += error * error;
        }

        const double gain = MU * error / energy;

        for (int k = 0; k < TAPS; ++k) {
            w[k] += gain * x[n - k];
        }
    }

    return {
        initial_sse / static_cast<double>(WINDOW),
        final_sse / static_cast<double>(WINDOW)
    };
}

/*
 * Independent numerical reference:
 * - Own coefficient array
 * - Independently regenerated desired echo signal
 * - Independent output and normalization calculations
 * - Different traversal order
 * - MSVC double arithmetic; no extended-precision claim
 * - Runs entirely outside the ROI
 */
static Metrics reference_nlms(double reference_w[TAPS])
{
    double reference_h[ECHO_TAPS];
    double reference_d[N];

    for (int k = 0; k < ECHO_TAPS; ++k) {
        const double position = static_cast<double>(k);
        const double decay = std::exp(-0.12 * position);
        const double oscillation = std::cos(0.37 * position);
        reference_h[k] = (0.7 * decay) * oscillation;
    }

    for (int n = 0; n < N; ++n) {
        double echo = 0.0;

        for (int k = ECHO_TAPS - 1; k >= 0; --k) {
            if (n >= k) {
                echo += x[n - k] * reference_h[k];
            }
        }

        reference_d[n] = echo;
    }

    for (int k = 0; k < TAPS; ++k) {
        reference_w[k] = 0.0;
    }

    double initial_sse = 0.0;
    double final_sse = 0.0;

    for (int n = FIRST; n < N; ++n) {
        double estimated_echo = 0.0;
        double input_power = 0.0;

        for (int k = TAPS - 1; k >= 0; --k) {
            const double sample = x[n - k];
            estimated_echo += sample * reference_w[k];
            input_power += sample * sample;
        }

        const double residual = reference_d[n] - estimated_echo;

        if (n >= FIRST && n <= INITIAL_LAST) {
            initial_sse += residual * residual;
        }

        if (n >= FINAL_FIRST && n < N) {
            final_sse += residual * residual;
        }

        const double denominator = EPSILON + input_power;
        const double correction = (MU / denominator) * residual;

        for (int k = TAPS - 1; k >= 0; --k) {
            reference_w[k] =
                reference_w[k] + x[n - k] * correction;
        }
    }

    return {
        initial_sse / static_cast<double>(WINDOW),
        final_sse / static_cast<double>(WINDOW)
    };
}

static bool close_mse(double a, double b)
{
    const double difference = std::fabs(a - b);
    const double scale = std::max(std::fabs(a), std::fabs(b));
    return difference <= 1.0e-10 + 1.0e-8 * scale;
}

int main()
{
    generate_input();
    generate_echo();

    if (!finite_array(x, N) ||
        !finite_array(d, N) ||
        !finite_array(h, ECHO_TAPS)) {
        std::printf("CP26_INPUT_GENERATION_FAIL\n");
        return 2;
    }

    Metrics measured = {0.0, 0.0};

    roi_begin();

    for (int run = 0; run < NLMS_RUNS; ++run) {
        for (int k = 0; k < TAPS; ++k) {
            w[k] = 0.0;
        }

        measured = production_nlms();
    }

    roi_end();

    double reference_w[TAPS];
    const Metrics reference = reference_nlms(reference_w);

    double weight_sse = 0.0;
    double max_weight_difference = 0.0;
    double path_sse = 0.0;

    for (int k = 0; k < TAPS; ++k) {
        const double diff = w[k] - reference_w[k];

        weight_sse += diff * diff;
        max_weight_difference =
            std::max(max_weight_difference, std::fabs(diff));

        const double true_coefficient =
            (k < ECHO_TAPS) ? h[k] : 0.0;

        const double path_diff = w[k] - true_coefficient;
        path_sse += path_diff * path_diff;
    }

    const double weight_rmse =
        std::sqrt(weight_sse / static_cast<double>(TAPS));

    const double path_rmse =
        std::sqrt(path_sse / static_cast<double>(TAPS));

    const bool finite_ok =
        finite_array(w, TAPS) &&
        finite_array(reference_w, TAPS) &&
        std::isfinite(measured.initial_mse) &&
        std::isfinite(measured.final_mse) &&
        std::isfinite(reference.initial_mse) &&
        std::isfinite(reference.final_mse) &&
        std::isfinite(weight_rmse) &&
        std::isfinite(max_weight_difference) &&
        std::isfinite(path_rmse);

    const bool numerical_ok =
        finite_ok &&
        max_weight_difference <= 1.0e-9 &&
        weight_rmse <= 1.0e-10 &&
        close_mse(measured.initial_mse, reference.initial_mse) &&
        close_mse(measured.final_mse, reference.final_mse);

    const bool hypothesis_mse =
        finite_ok && measured.final_mse <= 1.0e-3;

    const bool hypothesis_ratio =
        finite_ok &&
        measured.final_mse > 0.0 &&
        measured.initial_mse / measured.final_mse >= 10.0;

    const bool hypothesis_path =
        finite_ok && path_rmse <= 1.0e-2;

    std::printf("WORKLOAD=CP26_AEC_NLMS\n");
    std::printf("NLMS_RUNS=%d\n", NLMS_RUNS);
    std::printf("SIGNAL_LENGTH=%d\n", N);
    std::printf("ADAPTIVE_TAPS=%d\n", TAPS);
    std::printf("TRUE_ECHO_TAPS=%d\n", ECHO_TAPS);
    std::printf("NLMS_MU=%.17g\n", MU);
    std::printf("NLMS_EPSILON=%.17g\n", EPSILON);

    std::printf("INITIAL_MSE=%.17g\n", measured.initial_mse);
    std::printf("FINAL_MSE=%.17g\n", measured.final_mse);
    std::printf("REFERENCE_INITIAL_MSE=%.17g\n",
                reference.initial_mse);
    std::printf("REFERENCE_FINAL_MSE=%.17g\n",
                reference.final_mse);

    std::printf("MAX_WEIGHT_DIFFERENCE=%.17g\n",
                max_weight_difference);
    std::printf("REFERENCE_WEIGHT_RMSE=%.17g\n", weight_rmse);
    std::printf("TRUE_PATH_RMSE=%.17g\n", path_rmse);

    std::printf("FINITE_OK=%s\n", finite_ok ? "PASS" : "FAIL");
    std::printf("NUMERICAL_ORACLE=%s\n",
                numerical_ok ? "PASS" : "FAIL");

    std::printf("HYPOTHESIS_FINAL_MSE=%s\n",
                hypothesis_mse ? "PASS" : "FAIL");
    std::printf("HYPOTHESIS_MSE_RATIO=%s\n",
                hypothesis_ratio ? "PASS" : "FAIL");
    std::printf("HYPOTHESIS_PATH_RECOVERY=%s\n",
                hypothesis_path ? "PASS" : "FAIL");

    if (!numerical_ok) {
        std::printf("CP26_AEC_NLMS_1X_NATIVE_FAIL\n");
        return 2;
    }

    std::printf("CP26_AEC_NLMS_1X_NATIVE_OK\n");
    return 0;
}