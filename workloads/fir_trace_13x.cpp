#include <cmath>
#include <cstdint>
#include <cstdio>

static const int N = 4096;
static const int TAPS = 64;
static const int OUTPUT_COUNT = N - TAPS + 1;
static const int FIR_RUNS = 13;

static double input[FIR_RUNS][N];
static double output[FIR_RUNS][OUTPUT_COUNT];
static double coeff[TAPS];

extern "C" __declspec(dllexport) __declspec(noinline)
void roi_begin()
{
    volatile int marker = 1;
    (void)marker;
}

extern "C" __declspec(dllexport) __declspec(noinline)
void roi_end()
{
    volatile int marker = 2;
    (void)marker;
}

static void prepare_inputs()
{
    for (int r = 0; r < FIR_RUNS; ++r)
    {
        for (int n = 0; n < N; ++n)
        {
            input[r][n] = (double)(n + 1);
        }
    }
}

static void prepare_coefficients()
{
    for (int k = 0; k < TAPS; ++k)
    {
        coeff[k] = (double)(k + 1);
    }
}

static void fir_direct(const double* x, double* y)
{
    for (int out = 0; out < OUTPUT_COUNT; ++out)
    {
        int n = out + (TAPS - 1);

        double sum = 0.0;

        for (int k = 0; k < TAPS; ++k)
        {
            sum += coeff[k] * x[n - k];
        }

        y[out] = sum;
    }
}

static double expected_value(int out)
{
    int n = out + (TAPS - 1);

    return
        2080.0 * (double)(n + 1)
        - 87360.0;
}

static bool validate_fir(const double* y, int run)
{
    bool ok = true;
    double max_abs_error = 0.0;
    double checksum = 0.0;

    for (int out = 0; out < OUTPUT_COUNT; ++out)
    {
        double expected = expected_value(out);
        double error = std::fabs(y[out] - expected);

        if (error > max_abs_error)
            max_abs_error = error;

        if (y[out] != expected)
            ok = false;

        checksum += y[out];
    }

    printf("RUN%d_FIRST=%.0f\n", run, y[0]);
    printf("RUN%d_MIDDLE=%.0f\n", run, y[OUTPUT_COUNT / 2]);
    printf("RUN%d_LAST=%.0f\n", run, y[OUTPUT_COUNT - 1]);
    printf("RUN%d_CHECKSUM=%.0f\n", run, checksum);
    printf("RUN%d_MAX_ABS_ERROR=%.12e\n", run, max_abs_error);

    return ok;
}

int main()
{
    prepare_inputs();
    prepare_coefficients();

    printf("FIR_N=%d\n", N);
    printf("FIR_TAPS=%d\n", TAPS);
    printf("FIR_OUTPUT_COUNT=%d\n", OUTPUT_COUNT);
    printf("FIR_RUNS=%d\n", FIR_RUNS);

    for (int r = 0; r < FIR_RUNS; ++r)
    {
        printf("INPUT%d_BASE=%016llX\n",
               r,
               (unsigned long long)(uintptr_t)&input[r][0]);

        printf("OUTPUT%d_BASE=%016llX\n",
               r,
               (unsigned long long)(uintptr_t)&output[r][0]);
    }

    printf("COEFF_BASE=%016llX\n",
           (unsigned long long)(uintptr_t)&coeff[0]);

    roi_begin();

    for (int r = 0; r < FIR_RUNS; ++r)
    {
        fir_direct(input[r], output[r]);
    }

    roi_end();

    bool all_ok = true;

    for (int r = 0; r < FIR_RUNS; ++r)
    {
        bool run_ok = validate_fir(output[r], r);

        printf("RUN%d_OK=%d\n", r, run_ok ? 1 : 0);

        all_ok = all_ok && run_ok;
    }

    if (all_ok)
    {
        printf("FIR_13X_NATIVE_OK\n");
        return 0;
    }

    printf("FIR_13X_NATIVE_FAIL\n");
    return 1;
}
