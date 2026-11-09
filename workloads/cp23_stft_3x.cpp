#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

struct ComplexScalar {
    double re;
    double im;
};

static const int INPUT_LENGTH = 16384;
static const int SAMPLE_RATE_HZ = 16000;
static const int FRAME_LENGTH = 256;
static const int FFT_LENGTH = 256;
static const int HOP_LENGTH = 128;
static const int FRAME_COUNT = 127;
static const int ONE_SIDED_BINS = 129;
static const int OUTPUT_COUNT = FRAME_COUNT * ONE_SIDED_BINS;
static const int STFT_RUNS = 3;
static const double PI = 3.141592653589793238462643383279502884;

extern "C" __declspec(dllexport) __declspec(noinline) void roi_begin()
{
}

extern "C" __declspec(dllexport) __declspec(noinline) void roi_end()
{
}

static void fft_radix2_dit(ComplexScalar* x)
{
    for (unsigned i = 0; i < FFT_LENGTH; ++i) {
        unsigned r = 0;
        unsigned v = i;
        for (int b = 0; b < 8; ++b) {
            r = (r << 1) | (v & 1u);
            v >>= 1;
        }
        if (r > i) {
            ComplexScalar t = x[i];
            x[i] = x[r];
            x[r] = t;
        }
    }

    for (int len = 2; len <= FFT_LENGTH; len <<= 1) {
        const int half = len >> 1;
        const double theta = -2.0 * PI / static_cast<double>(len);
        const double step_re = std::cos(theta);
        const double step_im = std::sin(theta);

        for (int base = 0; base < FFT_LENGTH; base += len) {
            double w_re = 1.0;
            double w_im = 0.0;

            for (int j = 0; j < half; ++j) {
                const ComplexScalar even = x[base + j];
                const ComplexScalar odd = x[base + j + half];

                const double t_re = w_re * odd.re - w_im * odd.im;
                const double t_im = w_re * odd.im + w_im * odd.re;

                x[base + j].re = even.re + t_re;
                x[base + j].im = even.im + t_im;
                x[base + j + half].re = even.re - t_re;
                x[base + j + half].im = even.im - t_im;

                const double next_re = w_re * step_re - w_im * step_im;
                const double next_im = w_re * step_im + w_im * step_re;
                w_re = next_re;
                w_im = next_im;
            }
        }
    }
}

static void direct_dft_reference(
    const std::vector<double>& input,
    const std::vector<double>& window,
    std::vector<double>& reference)
{
    const double norm = static_cast<double>(FFT_LENGTH) *
                        static_cast<double>(FFT_LENGTH);

    for (int frame = 0; frame < FRAME_COUNT; ++frame) {
        const int start = frame * HOP_LENGTH;

        for (int k = 0; k < ONE_SIDED_BINS; ++k) {
            double sum_re = 0.0;
            double sum_im = 0.0;

            for (int n = 0; n < FRAME_LENGTH; ++n) {
                const double sample = input[start + n] * window[n];
                const double angle =
                    -2.0 * PI * static_cast<double>(k) *
                    static_cast<double>(n) /
                    static_cast<double>(FFT_LENGTH);

                sum_re += sample * std::cos(angle);
                sum_im += sample * std::sin(angle);
            }

            reference[frame * ONE_SIDED_BINS + k] =
                (sum_re * sum_re + sum_im * sum_im) / norm;
        }
    }
}

int main()
{
    std::vector<double> input(INPUT_LENGTH);
    std::vector<double> window(FRAME_LENGTH);
    std::vector<ComplexScalar> fft_buffer(FFT_LENGTH);
    std::vector<double> spectrogram(OUTPUT_COUNT);
    std::vector<double> reference(OUTPUT_COUNT);

    for (int n = 0; n < FRAME_LENGTH; ++n) {
        window[n] =
            0.5 -
            0.5 * std::cos(
                2.0 * PI * static_cast<double>(n) /
                static_cast<double>(FRAME_LENGTH - 1));
    }

    std::uint32_t lcg = 0x27182818u;
    for (int n = 0; n < INPUT_LENGTH; ++n) {
        lcg = lcg * 1664525u + 1013904223u;

        const double u =
            static_cast<double>((lcg >> 8) & 0x00FFFFFFu) /
            16777216.0;

        const double noise = 0.05 * (2.0 * u - 1.0);
        const double t =
            static_cast<double>(n) /
            static_cast<double>(SAMPLE_RATE_HZ);

        input[n] =
            0.6 * std::sin(2.0 * PI * 1000.0 * t) +
            0.3 * std::sin(2.0 * PI * 2500.0 * t) +
            noise;
    }

    const double norm =
        static_cast<double>(FFT_LENGTH) *
        static_cast<double>(FFT_LENGTH);

    roi_begin();

    for (int run = 0; run < STFT_RUNS; ++run) {
        for (int frame = 0; frame < FRAME_COUNT; ++frame) {
            const int start = frame * HOP_LENGTH;

            for (int n = 0; n < FFT_LENGTH; ++n) {
                fft_buffer[n].re = input[start + n] * window[n];
                fft_buffer[n].im = 0.0;
            }

            fft_radix2_dit(fft_buffer.data());

            for (int k = 0; k < ONE_SIDED_BINS; ++k) {
                const double re = fft_buffer[k].re;
                const double im = fft_buffer[k].im;

                spectrogram[frame * ONE_SIDED_BINS + k] =
                    (re * re + im * im) / norm;
            }
        }
    }

    roi_end();

    direct_dft_reference(input, window, reference);

    bool finite_ok = true;
    double max_abs_error = 0.0;
    double squared_error_sum = 0.0;

    for (int i = 0; i < OUTPUT_COUNT; ++i) {
        if (!std::isfinite(spectrogram[i]) ||
            !std::isfinite(reference[i])) {
            finite_ok = false;
        }

        const double diff = spectrogram[i] - reference[i];
        const double abs_error = std::fabs(diff);

        if (abs_error > max_abs_error) {
            max_abs_error = abs_error;
        }

        squared_error_sum += diff * diff;
    }

    const double rmse =
        std::sqrt(
            squared_error_sum /
            static_cast<double>(OUTPUT_COUNT));

    const bool frame_count_ok = (FRAME_COUNT == 127);
    const bool bin_count_ok = (ONE_SIDED_BINS == 129);
    const bool output_count_ok = (OUTPUT_COUNT == 16383);
    const bool max_abs_ok = (max_abs_error <= 1.0e-9);
    const bool rmse_ok = (rmse <= 1.0e-10);

    std::printf("STFT_RUNS=%d\n", STFT_RUNS);
    std::printf("FRAME_COUNT=%d\n", FRAME_COUNT);
    std::printf("ONE_SIDED_BIN_COUNT=%d\n", ONE_SIDED_BINS);
    std::printf("OUTPUT_COUNT=%d\n", OUTPUT_COUNT);
    std::printf("MAX_ABS_ERROR=%.17g\n", max_abs_error);
    std::printf("RMSE=%.17g\n", rmse);
    std::printf("ORACLE_FINITE=%s\n", finite_ok ? "PASS" : "FAIL");
    std::printf(
        "ORACLE_FRAME_COUNT=%s\n",
        frame_count_ok ? "PASS" : "FAIL");
    std::printf(
        "ORACLE_BIN_COUNT=%s\n",
        bin_count_ok ? "PASS" : "FAIL");
    std::printf(
        "ORACLE_OUTPUT_COUNT=%s\n",
        output_count_ok ? "PASS" : "FAIL");
    std::printf(
        "ORACLE_MAX_ABS_ERROR=%s\n",
        max_abs_ok ? "PASS" : "FAIL");
    std::printf(
        "ORACLE_RMSE=%s\n",
        rmse_ok ? "PASS" : "FAIL");

    const bool all_ok =
        finite_ok &&
        frame_count_ok &&
        bin_count_ok &&
        output_count_ok &&
        max_abs_ok &&
        rmse_ok;

    if (all_ok) {
        std::printf("CP23_STFT_NATIVE_OK\n");
        return 0;
    }

    std::printf("CP23_STFT_NATIVE_FAIL\n");
    return 2;
}