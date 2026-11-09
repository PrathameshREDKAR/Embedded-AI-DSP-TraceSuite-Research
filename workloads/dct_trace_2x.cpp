#include <cstdio>
#include <cstdint>
#include <cmath>

static const int WIDTH = 256;
static const int HEIGHT = 256;
static const int BLOCK = 8;
static const int DCT_RUNS = 2;

static double input_data[HEIGHT][WIDTH];
static double dct_output[HEIGHT][WIDTH];
static double reconstructed[HEIGHT][WIDTH];
static double coeff[BLOCK][BLOCK];

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
            input_data[r][c] =
                static_cast<double>((3 * r + 5 * c + 7) % 256);
        }
    }
}

static void init_coefficients()
{
    const double pi = 3.141592653589793238462643383279502884;

    for (int u = 0; u < BLOCK; ++u) {
        const double alpha =
            (u == 0) ? std::sqrt(1.0 / BLOCK)
                     : std::sqrt(2.0 / BLOCK);

        for (int x = 0; x < BLOCK; ++x) {
            coeff[u][x] =
                alpha *
                std::cos((pi * (2.0 * x + 1.0) * u) /
                         (2.0 * BLOCK));
        }
    }
}

__declspec(noinline)
static void dct_full_image()
{
    double temp[BLOCK][BLOCK];

    for (int br = 0; br < HEIGHT; br += BLOCK) {
        for (int bc = 0; bc < WIDTH; bc += BLOCK) {

            // First dimension: transform across columns.
            for (int r = 0; r < BLOCK; ++r) {
                for (int u = 0; u < BLOCK; ++u) {
                    double sum = 0.0;

                    for (int c = 0; c < BLOCK; ++c) {
                        sum += input_data[br + r][bc + c] *
                               coeff[u][c];
                    }

                    temp[r][u] = sum;
                }
            }

            // Second dimension: transform across rows.
            for (int v = 0; v < BLOCK; ++v) {
                for (int u = 0; u < BLOCK; ++u) {
                    double sum = 0.0;

                    for (int r = 0; r < BLOCK; ++r) {
                        sum += coeff[v][r] * temp[r][u];
                    }

                    dct_output[br + v][bc + u] = sum;
                }
            }
        }
    }
}

static void inverse_dct_full_image()
{
    double temp[BLOCK][BLOCK];

    for (int br = 0; br < HEIGHT; br += BLOCK) {
        for (int bc = 0; bc < WIDTH; bc += BLOCK) {

            // Undo row transform.
            for (int r = 0; r < BLOCK; ++r) {
                for (int u = 0; u < BLOCK; ++u) {
                    double sum = 0.0;

                    for (int v = 0; v < BLOCK; ++v) {
                        sum += coeff[v][r] *
                               dct_output[br + v][bc + u];
                    }

                    temp[r][u] = sum;
                }
            }

            // Undo column transform.
            for (int r = 0; r < BLOCK; ++r) {
                for (int c = 0; c < BLOCK; ++c) {
                    double sum = 0.0;

                    for (int u = 0; u < BLOCK; ++u) {
                        sum += temp[r][u] * coeff[u][c];
                    }

                    reconstructed[br + r][bc + c] = sum;
                }
            }
        }
    }
}

int main()
{
    init_input();
    init_coefficients();

    for (int r = 0; r < HEIGHT; ++r) {
        for (int c = 0; c < WIDTH; ++c) {
            dct_output[r][c] = 0.0;
            reconstructed[r][c] = 0.0;
        }
    }

    std::printf("DCT_WIDTH=%d\n", WIDTH);
    std::printf("DCT_HEIGHT=%d\n", HEIGHT);
    std::printf("DCT_BLOCK=%d\n", BLOCK);
    std::printf("DCT_RUNS=%d\n", DCT_RUNS);

    roi_begin();

    for (int run = 0; run < DCT_RUNS; ++run) {
        dct_full_image();
    }

    roi_end();

    inverse_dct_full_image();

    const double tolerance = 1e-9;
    double max_error = 0.0;
    std::uint64_t mismatches = 0;
    long double checksum = 0.0L;

    for (int r = 0; r < HEIGHT; ++r) {
        for (int c = 0; c < WIDTH; ++c) {
            const double error =
                std::fabs(reconstructed[r][c] - input_data[r][c]);

            if (error > max_error)
                max_error = error;

            if (error > tolerance)
                ++mismatches;

            const int weight = 1 + ((r * WIDTH + c) % 17);
            checksum +=
                static_cast<long double>(dct_output[r][c]) *
                static_cast<long double>(weight);
        }
    }

    std::printf("DCT_0_0=%.17g\n", dct_output[0][0]);
    std::printf("DCT_0_1=%.17g\n", dct_output[0][1]);
    std::printf("DCT_128_128=%.17g\n", dct_output[128][128]);
    std::printf("DCT_255_255=%.17g\n", dct_output[255][255]);
    std::printf("DCT_CHECKSUM=%.17Lg\n", checksum);
    std::printf("MAX_RECONSTRUCTION_ERROR=%.17g\n", max_error);
    std::printf("RECONSTRUCTION_MISMATCHES=%llu\n",
                static_cast<unsigned long long>(mismatches));

    if (mismatches != 0) {
        std::printf("DCT_1X_NATIVE_FAIL\n");
        return 1;
    }

    std::printf("DCT_1X_NATIVE_OK\n");
    return 0;
}
