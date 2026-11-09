#include <cstdio>
#include <cstdint>
#include <cstddef>

static constexpr std::size_t N = 262144;
static constexpr std::size_t IIR_RUNS = 14;

static double x[N];
static double y[N];

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

int main()
{
    for (std::size_t n = 0; n < N; ++n) {
        x[n] = static_cast<double>(n + 1);
        y[n] = 0.0;
    }

    roi_begin();

    for (std::size_t run = 0; run < IIR_RUNS; ++run) {
        double prev = 0.0;

        for (std::size_t n = 0; n < N; ++n) {
            const double current = x[n] + prev;
            y[n] = current;
            prev = current;
        }
    }

    roi_end();

    std::uint64_t mismatches = 0;
    double max_abs_error = 0.0;

    for (std::size_t n = 0; n < N; ++n) {
        const double a = static_cast<double>(n + 1);
        const double b = static_cast<double>(n + 2);
        const double expected = (a * b) / 2.0;

        const double diff =
            (y[n] >= expected) ? (y[n] - expected) : (expected - y[n]);

        if (diff != 0.0) {
            ++mismatches;

            if (diff > max_abs_error)
                max_abs_error = diff;
        }
    }

    const double expected_last =
        (static_cast<double>(N) * static_cast<double>(N + 1)) / 2.0;

    std::printf("IIR_N=%zu\n", N);
    std::printf("IIR_RUNS=%zu\n", IIR_RUNS);
    std::printf("Y_FIRST=%.0f\n", y[0]);
    std::printf("Y_LAST=%.0f\n", y[N - 1]);
    std::printf("EXPECTED_LAST=%.0f\n", expected_last);
    std::printf("MISMATCHES=%llu\n",
                static_cast<unsigned long long>(mismatches));
    std::printf("MAX_ABS_ERROR=%.0f\n", max_abs_error);

    if (mismatches == 0 && y[N - 1] == expected_last) {
        std::puts("IIR_14X_NATIVE_OK");
        return 0;
    }

    std::puts("IIR_14X_NATIVE_FAIL");
    return 1;
}
