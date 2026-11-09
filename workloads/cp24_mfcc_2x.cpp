#include <cmath>
#include <cstdint>
#include <cstdio>
#include <algorithm>

struct ComplexScalar {
    double re;
    double im;
};

constexpr int INPUT_SAMPLES = 16384;
constexpr int SAMPLE_RATE = 16000;
constexpr int FRAME_LENGTH = 400;
constexpr int HOP_LENGTH = 160;
constexpr int FRAME_COUNT = 100;
constexpr int FFT_LENGTH = 512;
constexpr int ONE_SIDED_BINS = 257;
constexpr int MEL_FILTER_COUNT = 26;
constexpr int MEL_POINT_COUNT = 28;
constexpr int MFCC_COEFFICIENT_COUNT = 13;
constexpr int OUTPUT_VALUES = FRAME_COUNT * MFCC_COEFFICIENT_COUNT;
constexpr int MFCC_RUNS = 2;

constexpr double PREEMPHASIS_ALPHA = 0.97;
constexpr double LOG_ENERGY_FLOOR = 1.0e-12;
constexpr double PI =
    3.141592653589793238462643383279502884;

static double g_input[INPUT_SAMPLES];
static double g_preemphasized[INPUT_SAMPLES];
static double g_reference_preemphasized[INPUT_SAMPLES];
static double g_window[FRAME_LENGTH];

static ComplexScalar g_fft[FFT_LENGTH];

static double g_power[ONE_SIDED_BINS];
static double g_logmel[MEL_FILTER_COUNT];
static int g_mel_bins[MEL_POINT_COUNT];

static double g_output[OUTPUT_VALUES];
static double g_reference[OUTPUT_VALUES];

extern "C" __declspec(dllexport) __declspec(noinline)
void roi_begin()
{
}

extern "C" __declspec(dllexport) __declspec(noinline)
void roi_end()
{
}

static double hz_to_mel(double hz)
{
    return 2595.0 * std::log10(1.0 + hz / 700.0);
}

static double mel_to_hz(double mel)
{
    return 700.0 * (std::pow(10.0, mel / 2595.0) - 1.0);
}

static void initialize_input()
{
    uint32_t state = 0x31415926u;

    for(int n = 0; n < INPUT_SAMPLES; ++n) {
        state = 1664525u * state + 1013904223u;

        const uint32_t r = (state >> 8) & 0x00ffffffu;
        const double u =
            static_cast<double>(r) / 16777215.0;
        const double noise =
            0.05 * (2.0 * u - 1.0);

        const double t =
            static_cast<double>(n) /
            static_cast<double>(SAMPLE_RATE);

        g_input[n] =
            0.50 * std::sin(2.0 * PI * 440.0 * t) +
            0.30 * std::sin(2.0 * PI * 1000.0 * t) +
            0.15 * std::sin(2.0 * PI * 2500.0 * t) +
            noise;
    }
}

static void initialize_window()
{
    for(int n = 0; n < FRAME_LENGTH; ++n) {
        g_window[n] =
            0.54 -
            0.46 *
            std::cos(
                2.0 * PI *
                static_cast<double>(n) /
                static_cast<double>(FRAME_LENGTH - 1));
    }
}

static void initialize_mel_bins()
{
    const double mel_low = hz_to_mel(0.0);
    const double mel_high = hz_to_mel(8000.0);

    for(int i = 0; i < MEL_POINT_COUNT; ++i) {
        const double fraction =
            static_cast<double>(i) /
            static_cast<double>(MEL_POINT_COUNT - 1);

        const double mel =
            mel_low + fraction * (mel_high - mel_low);

        const double hz = mel_to_hz(mel);

        int bin = static_cast<int>(
            std::floor(
                static_cast<double>(FFT_LENGTH + 1) *
                hz /
                static_cast<double>(SAMPLE_RATE)));

        if(bin < 0)
            bin = 0;

        if(bin > ONE_SIDED_BINS - 1)
            bin = ONE_SIDED_BINS - 1;

        g_mel_bins[i] = bin;
    }
}

static void preemphasize(
    const double* input,
    double* output)
{
    output[0] = input[0];

    for(int n = 1; n < INPUT_SAMPLES; ++n) {
        output[n] =
            input[n] -
            PREEMPHASIS_ALPHA * input[n - 1];
    }
}

static void fft_radix2_dit(ComplexScalar* a)
{
    for(unsigned int i = 1, j = 0;
        i < static_cast<unsigned int>(FFT_LENGTH);
        ++i) {

        unsigned int bit =
            static_cast<unsigned int>(FFT_LENGTH) >> 1;

        for(; j & bit; bit >>= 1)
            j ^= bit;

        j ^= bit;

        if(i < j) {
            const ComplexScalar tmp = a[i];
            a[i] = a[j];
            a[j] = tmp;
        }
    }

    for(int len = 2;
        len <= FFT_LENGTH;
        len <<= 1) {

        const double angle =
            -2.0 * PI /
            static_cast<double>(len);

        const ComplexScalar wlen = {
            std::cos(angle),
            std::sin(angle)
        };

        for(int i = 0;
            i < FFT_LENGTH;
            i += len) {

            ComplexScalar w = {1.0, 0.0};

            for(int j = 0;
                j < len / 2;
                ++j) {

                const ComplexScalar u = a[i + j];

                const ComplexScalar v = {
                    a[i + j + len / 2].re * w.re -
                    a[i + j + len / 2].im * w.im,

                    a[i + j + len / 2].re * w.im +
                    a[i + j + len / 2].im * w.re
                };

                a[i + j].re = u.re + v.re;
                a[i + j].im = u.im + v.im;

                a[i + j + len / 2].re = u.re - v.re;
                a[i + j + len / 2].im = u.im - v.im;

                const ComplexScalar next_w = {
                    w.re * wlen.re - w.im * wlen.im,
                    w.re * wlen.im + w.im * wlen.re
                };

                w = next_w;
            }
        }
    }
}

static double mel_filter_energy(
    const double* power,
    int filter_index)
{
    const int left =
        g_mel_bins[filter_index];

    const int center =
        g_mel_bins[filter_index + 1];

    const int right =
        g_mel_bins[filter_index + 2];

    double energy = 0.0;

    if(center > left) {
        for(int k = left; k < center; ++k) {
            const double weight =
                static_cast<double>(k - left) /
                static_cast<double>(center - left);

            energy += power[k] * weight;
        }
    }

    if(right > center) {
        for(int k = center; k <= right; ++k) {
            const double weight =
                static_cast<double>(right - k) /
                static_cast<double>(right - center);

            energy += power[k] * weight;
        }
    }

    return energy;
}

static void compute_one_mfcc(
    const double* input,
    double* output)
{
    preemphasize(input, g_preemphasized);

    for(int frame = 0;
        frame < FRAME_COUNT;
        ++frame) {

        const int frame_start =
            frame * HOP_LENGTH;

        for(int n = 0;
            n < FFT_LENGTH;
            ++n) {

            if(n < FRAME_LENGTH) {
                g_fft[n].re =
                    g_preemphasized[frame_start + n] *
                    g_window[n];

                g_fft[n].im = 0.0;
            }
            else {
                g_fft[n].re = 0.0;
                g_fft[n].im = 0.0;
            }
        }

        fft_radix2_dit(g_fft);

        for(int k = 0;
            k < ONE_SIDED_BINS;
            ++k) {

            g_power[k] =
                (g_fft[k].re * g_fft[k].re +
                 g_fft[k].im * g_fft[k].im) /
                static_cast<double>(FFT_LENGTH);
        }

        for(int m = 0;
            m < MEL_FILTER_COUNT;
            ++m) {

            double energy =
                mel_filter_energy(g_power, m);

            if(energy < LOG_ENERGY_FLOOR)
                energy = LOG_ENERGY_FLOOR;

            g_logmel[m] = std::log(energy);
        }

        for(int c = 0;
            c < MFCC_COEFFICIENT_COUNT;
            ++c) {

            double sum = 0.0;

            for(int m = 0;
                m < MEL_FILTER_COUNT;
                ++m) {

                sum +=
                    g_logmel[m] *
                    std::cos(
                        PI *
                        static_cast<double>(c) *
                        (static_cast<double>(m) + 0.5) /
                        static_cast<double>(MEL_FILTER_COUNT));
            }

            output[
                frame * MFCC_COEFFICIENT_COUNT + c
            ] = sum;
        }
    }
}

static void direct_dft_frame(
    const double* preemphasized,
    int frame_start,
    double* power)
{
    for(int k = 0;
        k < ONE_SIDED_BINS;
        ++k) {

        double re = 0.0;
        double im = 0.0;

        for(int n = 0;
            n < FRAME_LENGTH;
            ++n) {

            const double sample =
                preemphasized[frame_start + n] *
                g_window[n];

            const double angle =
                -2.0 * PI *
                static_cast<double>(k) *
                static_cast<double>(n) /
                static_cast<double>(FFT_LENGTH);

            re += sample * std::cos(angle);
            im += sample * std::sin(angle);
        }

        power[k] =
            (re * re + im * im) /
            static_cast<double>(FFT_LENGTH);
    }
}

static void compute_reference_mfcc(
    const double* input,
    double* output)
{
    preemphasize(
        input,
        g_reference_preemphasized);

    double power[ONE_SIDED_BINS];
    double logmel[MEL_FILTER_COUNT];

    for(int frame = 0;
        frame < FRAME_COUNT;
        ++frame) {

        const int frame_start =
            frame * HOP_LENGTH;

        direct_dft_frame(
            g_reference_preemphasized,
            frame_start,
            power);

        for(int m = 0;
            m < MEL_FILTER_COUNT;
            ++m) {

            double energy =
                mel_filter_energy(power, m);

            if(energy < LOG_ENERGY_FLOOR)
                energy = LOG_ENERGY_FLOOR;

            logmel[m] = std::log(energy);
        }

        for(int c = 0;
            c < MFCC_COEFFICIENT_COUNT;
            ++c) {

            double sum = 0.0;

            for(int m = 0;
                m < MEL_FILTER_COUNT;
                ++m) {

                sum +=
                    logmel[m] *
                    std::cos(
                        PI *
                        static_cast<double>(c) *
                        (static_cast<double>(m) + 0.5) /
                        static_cast<double>(MEL_FILTER_COUNT));
            }

            output[
                frame * MFCC_COEFFICIENT_COUNT + c
            ] = sum;
        }
    }
}

int main()
{
    initialize_input();
    initialize_window();
    initialize_mel_bins();

    roi_begin();

    for(int run = 0;
        run < MFCC_RUNS;
        ++run) {

        compute_one_mfcc(
            g_input,
            g_output);
    }

    roi_end();

    compute_reference_mfcc(
        g_input,
        g_reference);

    bool finite_pass = true;
    double max_abs_error = 0.0;
    double squared_error_sum = 0.0;
    double checksum = 0.0;

    for(int i = 0;
        i < OUTPUT_VALUES;
        ++i) {

        if(!std::isfinite(g_output[i]) ||
           !std::isfinite(g_reference[i])) {
            finite_pass = false;
        }

        const double diff =
            g_output[i] - g_reference[i];

        const double abs_diff =
            std::fabs(diff);

        if(abs_diff > max_abs_error)
            max_abs_error = abs_diff;

        squared_error_sum += diff * diff;

        checksum +=
            g_output[i] *
            static_cast<double>(i + 1);
    }

    const double rmse =
        std::sqrt(
            squared_error_sum /
            static_cast<double>(OUTPUT_VALUES));

    const bool oracle_pass =
        finite_pass &&
        max_abs_error <= 1.0e-8 &&
        rmse <= 1.0e-9;

    std::printf("CP24_WORKLOAD=MFCC\n");
    std::printf("MFCC_RUNS=%d\n", MFCC_RUNS);
    std::printf("FRAME_COUNT=%d\n", FRAME_COUNT);
    std::printf("FFT_LENGTH=%d\n", FFT_LENGTH);
    std::printf("ONE_SIDED_BINS=%d\n", ONE_SIDED_BINS);
    std::printf("MEL_FILTER_COUNT=%d\n", MEL_FILTER_COUNT);
    std::printf(
        "MFCC_COEFFICIENT_COUNT=%d\n",
        MFCC_COEFFICIENT_COUNT);
    std::printf(
        "OUTPUT_VALUES=%d\n",
        OUTPUT_VALUES);
    std::printf(
        "FINITE=%s\n",
        finite_pass ? "PASS" : "FAIL");
    std::printf(
        "MAX_ABS_ERROR=%.17g\n",
        max_abs_error);
    std::printf(
        "RMSE=%.17g\n",
        rmse);
    std::printf(
        "CHECKSUM=%.17g\n",
        checksum);
    std::printf(
        "ORACLE=%s\n",
        oracle_pass ? "PASS" : "FAIL");

    if(!oracle_pass)
        return 2;

    std::printf("CP24_MFCC_NATIVE_OK\n");
    return 0;
}