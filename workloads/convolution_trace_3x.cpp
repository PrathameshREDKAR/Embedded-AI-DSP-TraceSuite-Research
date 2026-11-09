#include <cmath>
#include <cstdint>
#include <cstdio>

static const int HEIGHT = 256;
static const int WIDTH = 256;
static const int KERNEL_H = 3;
static const int KERNEL_W = 3;

static const int OUTPUT_H = HEIGHT - KERNEL_H + 1;
static const int OUTPUT_W = WIDTH - KERNEL_W + 1;

static const int CONV_RUNS = 3;

static double input[CONV_RUNS][HEIGHT][WIDTH];
static double output[CONV_RUNS][OUTPUT_H][OUTPUT_W];
static double kernel[KERNEL_H][KERNEL_W];

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
    for (int r = 0; r < CONV_RUNS; ++r)
    {
        for (int y = 0; y < HEIGHT; ++y)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                input[r][y][x] =
                    (double)(y * WIDTH + x + 1);
            }
        }
    }
}

static void prepare_kernel()
{
    int value = 1;

    for (int ky = 0; ky < KERNEL_H; ++ky)
    {
        for (int kx = 0; kx < KERNEL_W; ++kx)
        {
            kernel[ky][kx] = (double)value;
            ++value;
        }
    }
}

static void convolution_direct(
    const double src[HEIGHT][WIDTH],
    double dst[OUTPUT_H][OUTPUT_W])
{
    for (int oy = 0; oy < OUTPUT_H; ++oy)
    {
        for (int ox = 0; ox < OUTPUT_W; ++ox)
        {
            double sum = 0.0;

            for (int ky = 0; ky < KERNEL_H; ++ky)
            {
                for (int kx = 0; kx < KERNEL_W; ++kx)
                {
                    sum +=
                        kernel[ky][kx] *
                        src[oy + ky][ox + kx];
                }
            }

            dst[oy][ox] = sum;
        }
    }
}

static double expected_value(int oy, int ox)
{
    return
        45.0 * (double)(oy * WIDTH + ox + 1)
        + 63.0 * (double)WIDTH
        + 51.0;
}

static bool validate_convolution(
    const double dst[OUTPUT_H][OUTPUT_W],
    int run)
{
    bool ok = true;
    double max_abs_error = 0.0;
    double checksum = 0.0;

    for (int oy = 0; oy < OUTPUT_H; ++oy)
    {
        for (int ox = 0; ox < OUTPUT_W; ++ox)
        {
            double expected = expected_value(oy, ox);
            double error = std::fabs(dst[oy][ox] - expected);

            if (error > max_abs_error)
                max_abs_error = error;

            if (dst[oy][ox] != expected)
                ok = false;

            checksum += dst[oy][ox];
        }
    }

    printf("RUN%d_FIRST=%.0f\n",
           run,
           dst[0][0]);

    printf("RUN%d_CENTER=%.0f\n",
           run,
           dst[OUTPUT_H / 2][OUTPUT_W / 2]);

    printf("RUN%d_LAST=%.0f\n",
           run,
           dst[OUTPUT_H - 1][OUTPUT_W - 1]);

    printf("RUN%d_CHECKSUM=%.0f\n",
           run,
           checksum);

    printf("RUN%d_MAX_ABS_ERROR=%.12e\n",
           run,
           max_abs_error);

    return ok;
}

int main()
{
    prepare_inputs();
    prepare_kernel();

    printf("CONV_HEIGHT=%d\n", HEIGHT);
    printf("CONV_WIDTH=%d\n", WIDTH);
    printf("CONV_KERNEL_H=%d\n", KERNEL_H);
    printf("CONV_KERNEL_W=%d\n", KERNEL_W);
    printf("CONV_OUTPUT_H=%d\n", OUTPUT_H);
    printf("CONV_OUTPUT_W=%d\n", OUTPUT_W);
    printf("CONV_RUNS=%d\n", CONV_RUNS);

    for (int r = 0; r < CONV_RUNS; ++r)
    {
        printf("INPUT%d_BASE=%016llX\n",
               r,
               (unsigned long long)(uintptr_t)&input[r][0][0]);

        printf("OUTPUT%d_BASE=%016llX\n",
               r,
               (unsigned long long)(uintptr_t)&output[r][0][0]);
    }

    printf("KERNEL_BASE=%016llX\n",
           (unsigned long long)(uintptr_t)&kernel[0][0]);

    roi_begin();

    for (int r = 0; r < CONV_RUNS; ++r)
    {
        convolution_direct(input[r], output[r]);
    }

    roi_end();

    bool all_ok = true;

    for (int r = 0; r < CONV_RUNS; ++r)
    {
        bool run_ok =
            validate_convolution(output[r], r);

        printf("RUN%d_OK=%d\n",
               r,
               run_ok ? 1 : 0);

        all_ok = all_ok && run_ok;
    }

    if (all_ok)
    {
        printf("CONVOLUTION_3X_NATIVE_OK\n");
        return 0;
    }

    printf("CONVOLUTION_3X_NATIVE_FAIL\n");
    return 1;
}
