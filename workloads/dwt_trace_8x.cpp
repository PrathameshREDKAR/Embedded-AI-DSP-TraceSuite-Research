#include <cstdio>
#include <cstdint>
#include <cmath>

static const int WIDTH = 256;
static const int HEIGHT = 256;
static const int LEVELS = 4;
static const int DWT_RUNS = 8;

static double pristine_input[HEIGHT][WIDTH];
static double work_data[HEIGHT][WIDTH];
static double reconstructed[HEIGHT][WIDTH];
static double scratch[WIDTH];

static volatile double anti_opt_sink = 0.0;

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

static void init_input()
{
    for (int r = 0; r < HEIGHT; ++r) {
        for (int c = 0; c < WIDTH; ++c) {
            pristine_input[r][c] =
                static_cast<double>((3 * r + 5 * c + 7) % 256);
        }
    }
}

__declspec(noinline)
static void copy_pristine_to_work()
{
    for (int r = 0; r < HEIGHT; ++r) {
        for (int c = 0; c < WIDTH; ++c) {
            work_data[r][c] = pristine_input[r][c];
        }
    }
}

__declspec(noinline)
static void forward_haar_2d()
{
    const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
    int active = WIDTH;

    for (int level = 0; level < LEVELS; ++level) {
        const int half = active / 2;

        // Row transform over the current top-left active region.
        for (int r = 0; r < active; ++r) {
            for (int k = 0; k < half; ++k) {
                const double a = work_data[r][2 * k];
                const double b = work_data[r][2 * k + 1];
                scratch[k] = (a + b) * inv_sqrt2;
                scratch[half + k] = (a - b) * inv_sqrt2;
            }

            for (int c = 0; c < active; ++c) {
                work_data[r][c] = scratch[c];
            }
        }

        // Column transform over the current top-left active region.
        for (int c = 0; c < active; ++c) {
            for (int k = 0; k < half; ++k) {
                const double a = work_data[2 * k][c];
                const double b = work_data[2 * k + 1][c];
                scratch[k] = (a + b) * inv_sqrt2;
                scratch[half + k] = (a - b) * inv_sqrt2;
            }

            for (int r = 0; r < active; ++r) {
                work_data[r][c] = scratch[r];
            }
        }

        active /= 2;
    }
}

static void copy_work_to_reconstructed()
{
    for (int r = 0; r < HEIGHT; ++r) {
        for (int c = 0; c < WIDTH; ++c) {
            reconstructed[r][c] = work_data[r][c];
        }
    }
}

static void inverse_haar_2d()
{
    const double inv_sqrt2 = 1.0 / std::sqrt(2.0);
    int active = WIDTH >> (LEVELS - 1);

    for (int level = LEVELS - 1; level >= 0; --level) {
        const int half = active / 2;

        // Undo the column transform first.
        for (int c = 0; c < active; ++c) {
            for (int k = 0; k < half; ++k) {
                const double a = reconstructed[k][c];
                const double d = reconstructed[half + k][c];
                scratch[2 * k] = (a + d) * inv_sqrt2;
                scratch[2 * k + 1] = (a - d) * inv_sqrt2;
            }

            for (int r = 0; r < active; ++r) {
                reconstructed[r][c] = scratch[r];
            }
        }

        // Undo the row transform second.
        for (int r = 0; r < active; ++r) {
            for (int k = 0; k < half; ++k) {
                const double a = reconstructed[r][k];
                const double d = reconstructed[r][half + k];
                scratch[2 * k] = (a + d) * inv_sqrt2;
                scratch[2 * k + 1] = (a - d) * inv_sqrt2;
            }

            for (int c = 0; c < active; ++c) {
                reconstructed[r][c] = scratch[c];
            }
        }

        active *= 2;
    }
}

int main()
{
    init_input();

    for (int r = 0; r < HEIGHT; ++r) {
        for (int c = 0; c < WIDTH; ++c) {
            work_data[r][c] = 0.0;
            reconstructed[r][c] = 0.0;
        }
    }

    std::printf("DWT_WIDTH=%d\n", WIDTH);
    std::printf("DWT_HEIGHT=%d\n", HEIGHT);
    std::printf("DWT_LEVELS=%d\n", LEVELS);
    std::printf("DWT_RUNS=%d\n", DWT_RUNS);

    roi_begin();

    for (int run = 0; run < DWT_RUNS; ++run) {
        copy_pristine_to_work();
        forward_haar_2d();

        anti_opt_sink +=
            work_data[0][0] +
            work_data[0][1] +
            work_data[HEIGHT - 1][WIDTH - 1];
    }

    roi_end();

    long double checksum = 0.0L;
    for (int r = 0; r < HEIGHT; ++r) {
        for (int c = 0; c < WIDTH; ++c) {
            const int weight = 1 + ((r * WIDTH + c) % 17);
            checksum +=
                static_cast<long double>(work_data[r][c]) *
                static_cast<long double>(weight);
        }
    }

    copy_work_to_reconstructed();
    inverse_haar_2d();

    const double tolerance = 1e-9;
    double max_error = 0.0;
    std::uint64_t mismatches = 0;

    for (int r = 0; r < HEIGHT; ++r) {
        for (int c = 0; c < WIDTH; ++c) {
            const double error =
                std::fabs(reconstructed[r][c] - pristine_input[r][c]);

            if (error > max_error)
                max_error = error;

            if (error > tolerance)
                ++mismatches;
        }
    }

    std::printf("DWT_0_0=%.17g\n", work_data[0][0]);
    std::printf("DWT_0_1=%.17g\n", work_data[0][1]);
    std::printf("DWT_128_128=%.17g\n", work_data[128][128]);
    std::printf("DWT_255_255=%.17g\n", work_data[255][255]);
    std::printf("DWT_CHECKSUM=%.17Lg\n", checksum);
    std::printf("DWT_SINK=%.17g\n", static_cast<double>(anti_opt_sink));
    std::printf("MAX_RECONSTRUCTION_ERROR=%.17g\n", max_error);
    std::printf("RECONSTRUCTION_MISMATCHES=%llu\n",
                static_cast<unsigned long long>(mismatches));

    if (mismatches != 0) {
        std::printf("DWT_1X_NATIVE_FAIL\n");
        return 1;
    }

    std::printf("DWT_1X_NATIVE_OK\n");
    return 0;
}
