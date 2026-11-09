#include <cmath>
#include <cstdint>
#include <cstdio>

static const int N = 1024;
static const int TONE_BIN = 7;
static const int FFT_RUNS = 100;
static const double PI = 3.141592653589793238462643383279502884;

struct Complex
{
    double re;
    double im;
};

static Complex data[FFT_RUNS][N];
static Complex twiddle[N / 2];

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
    for (int r = 0; r < FFT_RUNS; ++r)
    {
        for (int n = 0; n < N; ++n)
        {
            double angle = 2.0 * PI * TONE_BIN * n / N;
            data[r][n].re = std::cos(angle);
            data[r][n].im = std::sin(angle);
        }
    }
}

static void prepare_twiddles()
{
    for (int k = 0; k < N / 2; ++k)
    {
        double angle = -2.0 * PI * k / N;
        twiddle[k].re = std::cos(angle);
        twiddle[k].im = std::sin(angle);
    }
}

static unsigned reverse_bits(unsigned x, int bits)
{
    unsigned y = 0;

    for (int i = 0; i < bits; ++i)
    {
        y = (y << 1) | (x & 1U);
        x >>= 1;
    }

    return y;
}

static void bit_reverse_permutation(Complex* a)
{
    const int bits = 10;

    for (unsigned i = 0; i < (unsigned)N; ++i)
    {
        unsigned j = reverse_bits(i, bits);

        if (j > i)
        {
            Complex temp = a[i];
            a[i] = a[j];
            a[j] = temp;
        }
    }
}

static void fft_radix2(Complex* a)
{
    bit_reverse_permutation(a);

    for (int len = 2; len <= N; len <<= 1)
    {
        int half = len >> 1;
        int twiddle_step = N / len;

        for (int base = 0; base < N; base += len)
        {
            for (int j = 0; j < half; ++j)
            {
                Complex u = a[base + j];
                Complex v = a[base + j + half];
                Complex w = twiddle[j * twiddle_step];

                Complex t;
                t.re = w.re * v.re - w.im * v.im;
                t.im = w.re * v.im + w.im * v.re;

                a[base + j].re = u.re + t.re;
                a[base + j].im = u.im + t.im;

                a[base + j + half].re = u.re - t.re;
                a[base + j + half].im = u.im - t.im;
            }
        }
    }
}

static double magnitude_squared(const Complex& x)
{
    return x.re * x.re + x.im * x.im;
}

static bool validate_fft(const Complex* a, int run)
{
    int peak_bin = 0;
    double peak_mag2 = magnitude_squared(a[0]);
    double max_other_mag2 = 0.0;
    double spectral_energy = 0.0;

    for (int k = 0; k < N; ++k)
    {
        double mag2 = magnitude_squared(a[k]);
        spectral_energy += mag2;

        if (mag2 > peak_mag2)
        {
            peak_mag2 = mag2;
            peak_bin = k;
        }
    }

    for (int k = 0; k < N; ++k)
    {
        if (k == TONE_BIN)
            continue;

        double mag2 = magnitude_squared(a[k]);
        if (mag2 > max_other_mag2)
            max_other_mag2 = mag2;
    }

    double peak_error_re = std::fabs(a[TONE_BIN].re - (double)N);
    double peak_error_im = std::fabs(a[TONE_BIN].im);
    double max_other_mag = std::sqrt(max_other_mag2);
    double expected_energy = (double)N * (double)N;
    double energy_error = std::fabs(spectral_energy - expected_energy);

    printf("RUN%d_PEAK_BIN=%d\n", run, peak_bin);
    printf("RUN%d_PEAK_RE=%.12f\n", run, a[TONE_BIN].re);
    printf("RUN%d_PEAK_IM=%.12f\n", run, a[TONE_BIN].im);
    printf("RUN%d_MAX_OTHER_MAG=%.12e\n", run, max_other_mag);
    printf("RUN%d_ENERGY=%.12f\n", run, spectral_energy);

    return
        peak_bin == TONE_BIN &&
        peak_error_re < 1.0e-6 &&
        peak_error_im < 1.0e-6 &&
        max_other_mag < 1.0e-6 &&
        energy_error < 1.0e-5;
}

int main()
{
    prepare_inputs();
    prepare_twiddles();

    printf("FFT_N=%d\n", N);
    printf("FFT_RUNS=%d\n", FFT_RUNS);
    printf("EXPECTED_PEAK_BIN=%d\n", TONE_BIN);
    for (int r = 0; r < FFT_RUNS; ++r)
    {
        printf("DATA%d_BASE=%016llX\n",
               r,
               (unsigned long long)(uintptr_t)&data[r][0]);
    }
    printf("TWIDDLE_BASE=%016llX\n",
           (unsigned long long)(uintptr_t)&twiddle[0]);

    roi_begin();

    for (int r = 0; r < FFT_RUNS; ++r)
    {
        fft_radix2(data[r]);
    }

    roi_end();

    bool all_ok = true;

    for (int r = 0; r < FFT_RUNS; ++r)
    {
        bool run_ok = validate_fft(data[r], r);
        printf("RUN%d_OK=%d\n", r, run_ok ? 1 : 0);
        all_ok = all_ok && run_ok;
    }

    if (all_ok)
    {
        printf("FFT_100X_NATIVE_OK\n");
        return 0;
    }

    printf("FFT_100X_NATIVE_FAIL\n");
    return 1;
}
