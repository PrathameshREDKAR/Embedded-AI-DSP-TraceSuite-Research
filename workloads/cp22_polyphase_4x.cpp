#include <cmath>
#include <cstdint>
#include <cstdio>

extern "C" __declspec(dllexport) __declspec(noinline) void roi_begin() {}
extern "C" __declspec(dllexport) __declspec(noinline) void roi_end() {}

static const int INPUT_N = 16384;
static const int L = 3;
static const int M = 2;
static const int TAPS = 63;
static const int TAPS_PER_PHASE = 21;
static const int CENTER = 31;
static const int OUTPUT_N = 24606;
static const int RESAMPLE_RUNS = 4;

static double g_input[INPUT_N];
static double g_h[TAPS];
static double g_output[OUTPUT_N];
static double g_reference[OUTPUT_N];

static void init_input()
{
    uint32_t state = 0x31415926u;
    for (int n = 0; n < INPUT_N; ++n) {
        state = 1664525u * state + 1013904223u;
        const double u = static_cast<double>(state >> 8) / 16777215.0;
        g_input[n] = 2.0 * u - 1.0;
    }
}

static void init_filter()
{
    const double pi = 3.141592653589793238462643383279502884;
    const double fc = 0.15;
    double sum = 0.0;

    for (int n = 0; n < TAPS; ++n) {
        const int k = n - CENTER;
        double ideal;

        if (k == 0)
            ideal = 2.0 * fc;
        else
            ideal = std::sin(2.0 * pi * fc * static_cast<double>(k))
                  / (pi * static_cast<double>(k));

        const double window =
            0.54 - 0.46 * std::cos(2.0 * pi * static_cast<double>(n)
                                  / static_cast<double>(TAPS - 1));

        g_h[n] = ideal * window;
        sum += g_h[n];
    }

    const double scale = static_cast<double>(L) / sum;
    for (int n = 0; n < TAPS; ++n)
        g_h[n] *= scale;
}

static void compute_reference()
{
    for (int m = 0; m < OUTPUT_N; ++m) {
        const long long t = static_cast<long long>(m) * M;
        double acc = 0.0;

        for (int r = 0; r < TAPS; ++r) {
            const long long numerator = t - r;

            if (numerator < 0)
                continue;
            if ((numerator % L) != 0)
                continue;

            const long long n = numerator / L;
            if (n >= 0 && n < INPUT_N)
                acc += g_h[r] * g_input[n];
        }

        g_reference[m] = acc;
    }
}

__declspec(noinline) static void polyphase_resample()
{
    for (int m = 0; m < OUTPUT_N; ++m) {
        const long long t = static_cast<long long>(m) * M;
        const int phase = static_cast<int>(t % L);
        const long long n0 = t / L;
        double acc = 0.0;

        for (int q = 0; q < TAPS_PER_PHASE; ++q) {
            const int r = phase + q * L;
            if (r >= TAPS)
                continue;

            const long long n = n0 - q;
            if (n >= 0 && n < INPUT_N)
                acc += g_h[r] * g_input[n];
        }

        g_output[m] = acc;
    }
}

int main()
{
    static_assert(OUTPUT_N == (((INPUT_N - 1) * L + (TAPS - 1)) / M + 1),
                  "Unexpected output length");

    init_input();
    init_filter();
    compute_reference();

    roi_begin();
    for (int run = 0; run < RESAMPLE_RUNS; ++run)
        polyphase_resample();
    roi_end();

    bool finite_ok = true;

    for (int n = 0; n < INPUT_N; ++n)
        finite_ok = finite_ok && std::isfinite(g_input[n]);

    double coeff_sum = 0.0;
    for (int r = 0; r < TAPS; ++r) {
        finite_ok = finite_ok && std::isfinite(g_h[r]);
        coeff_sum += g_h[r];
    }

    double max_abs_error = 0.0;
    double squared_error_sum = 0.0;

    for (int m = 0; m < OUTPUT_N; ++m) {
        finite_ok = finite_ok
                 && std::isfinite(g_output[m])
                 && std::isfinite(g_reference[m]);

        const double err = g_output[m] - g_reference[m];
        const double abs_err = std::fabs(err);

        if (abs_err > max_abs_error)
            max_abs_error = abs_err;

        squared_error_sum += err * err;
    }

    const double rmse =
        std::sqrt(squared_error_sum / static_cast<double>(OUTPUT_N));
    const double coeff_sum_abs_error =
        std::fabs(coeff_sum - static_cast<double>(L));

    finite_ok = finite_ok
             && std::isfinite(coeff_sum)
             && std::isfinite(coeff_sum_abs_error)
             && std::isfinite(max_abs_error)
             && std::isfinite(rmse);

    const bool output_count_ok = (OUTPUT_N == 24606);
    const bool coeff_ok = (coeff_sum_abs_error <= 1.0e-12);
    const bool max_abs_ok = (max_abs_error <= 1.0e-10);
    const bool rmse_ok = (rmse <= 1.0e-11);

    std::printf("RESAMPLE_RUNS=%d\n", RESAMPLE_RUNS);
    std::printf("OUTPUT_COUNT=%d\n", OUTPUT_N);
    std::printf("COEFFICIENT_SUM=%.17g\n", coeff_sum);
    std::printf("COEFFICIENT_SUM_ABS_ERROR=%.17g\n", coeff_sum_abs_error);
    std::printf("MAX_ABS_ERROR=%.17g\n", max_abs_error);
    std::printf("RMSE=%.17g\n", rmse);
    std::printf("ORACLE_FINITE=%s\n", finite_ok ? "PASS" : "FAIL");
    std::printf("ORACLE_OUTPUT_COUNT=%s\n", output_count_ok ? "PASS" : "FAIL");
    std::printf("ORACLE_COEFFICIENT_SUM=%s\n", coeff_ok ? "PASS" : "FAIL");
    std::printf("ORACLE_MAX_ABS_ERROR=%s\n", max_abs_ok ? "PASS" : "FAIL");
    std::printf("ORACLE_RMSE=%s\n", rmse_ok ? "PASS" : "FAIL");

    const bool all_ok =
        finite_ok && output_count_ok && coeff_ok && max_abs_ok && rmse_ok;

    if (all_ok) {
        std::printf("POLYPHASE_1X_NATIVE_OK\n");
        return 0;
    }

    std::printf("POLYPHASE_1X_NATIVE_FAIL\n");
    return 1;
}