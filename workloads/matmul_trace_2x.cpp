#include <cstdio>
#include <cstdint>
#include <cmath>

static const int N = 128;
static const int MATMUL_RUNS = 2;

static double A[N][N];
static double B[N][N];
static double C[N][N];

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

__declspec(noinline)
static void matmul()
{
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            double sum = 0.0;

            for (int k = 0; k < N; ++k) {
                sum += A[i][k] * B[k][j];
            }

            C[i][j] = sum;
        }
    }
}

static std::uint64_t expected_value(int i, int j)
{
    const std::uint64_t n  = N;
    const std::uint64_t s1 = n * (n - 1) / 2;
    const std::uint64_t s2 = n * (n - 1) * (2 * n - 1) / 6;

    return n * static_cast<std::uint64_t>(i + 1)
             * static_cast<std::uint64_t>(j + 1)
         + static_cast<std::uint64_t>(i + j + 2) * s1
         + s2;
}

int main()
{
    for (int i = 0; i < N; ++i) {
        for (int k = 0; k < N; ++k) {
            A[i][k] = static_cast<double>(i + k + 1);
        }
    }

    for (int k = 0; k < N; ++k) {
        for (int j = 0; j < N; ++j) {
            B[k][j] = static_cast<double>(k + j + 1);
        }
    }

    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            C[i][j] = 0.0;

    std::printf("MATMUL_N=%d\n", N);
    std::printf("MATMUL_RUNS=%d\n", MATMUL_RUNS);

    roi_begin();

    for (int run = 0; run < MATMUL_RUNS; ++run) {
        matmul();
    }

    roi_end();

    std::uint64_t checksum = 0;
    int mismatches = 0;
    double max_abs_error = 0.0;

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            const std::uint64_t expected_int = expected_value(i, j);
            const double expected = static_cast<double>(expected_int);
            const double err = std::fabs(C[i][j] - expected);

            if (err != 0.0)
                ++mismatches;

            if (err > max_abs_error)
                max_abs_error = err;

            checksum += static_cast<std::uint64_t>(C[i][j]);
        }
    }

    std::printf("C_FIRST=%.0f\n", C[0][0]);
    std::printf("C_CENTER=%.0f\n", C[N/2][N/2]);
    std::printf("C_LAST=%.0f\n", C[N-1][N-1]);
    std::printf("CHECKSUM=%llu\n",
                static_cast<unsigned long long>(checksum));
    std::printf("MISMATCHES=%d\n", mismatches);
    std::printf("MAX_ABS_ERROR=%.17g\n", max_abs_error);

    if (mismatches != 0) {
        std::printf("MATMUL_1X_NATIVE_FAIL\n");
        return 1;
    }

    std::printf("MATMUL_1X_NATIVE_OK\n");
    return 0;
}
