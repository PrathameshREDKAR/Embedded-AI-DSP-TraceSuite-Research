#include <cmath>
#include <cstdint>
#include <cstdio>

constexpr int SAMPLE_RATE = 16000;
constexpr int INPUT_SAMPLES_PER_MIC = 16384;
constexpr int MICROPHONE_COUNT = 8;
constexpr int BEAM_COUNT = 31;
constexpr int OUTPUT_SAMPLES_PER_BEAM = INPUT_SAMPLES_PER_MIC;
constexpr int OUTPUT_VALUES =
    BEAM_COUNT * OUTPUT_SAMPLES_PER_BEAM;
constexpr int BEAMFORM_RUNS = 1;

constexpr double MICROPHONE_SPACING_METERS = 0.035;
constexpr double SOUND_SPEED_METERS_PER_SECOND = 343.0;

constexpr double BEAM_ANGLE_START_DEG = -60.0;
constexpr double BEAM_ANGLE_STEP_DEG = 4.0;

constexpr double DESIRED_SOURCE_ANGLE_DEG = 20.0;
constexpr double DESIRED_SOURCE_FREQUENCY_HZ = 700.0;
constexpr double DESIRED_SOURCE_AMPLITUDE = 0.60;

constexpr double INTERFERER_SOURCE_ANGLE_DEG = -36.0;
constexpr double INTERFERER_SOURCE_FREQUENCY_HZ = 1900.0;
constexpr double INTERFERER_SOURCE_AMPLITUDE = 0.25;

constexpr double NOISE_AMPLITUDE = 0.02;

constexpr double PI =
    3.141592653589793238462643383279502884;

constexpr long double PI_L =
    3.141592653589793238462643383279502884L;

static double g_input[MICROPHONE_COUNT][INPUT_SAMPLES_PER_MIC];

static double g_delay_samples[BEAM_COUNT][MICROPHONE_COUNT];

static double g_output[OUTPUT_VALUES];
static long double g_reference[OUTPUT_VALUES];

extern "C" __declspec(dllexport) __declspec(noinline)
void roi_begin()
{
}

extern "C" __declspec(dllexport) __declspec(noinline)
void roi_end()
{
}

static double degrees_to_radians(double degrees)
{
    return degrees * PI / 180.0;
}

static long double degrees_to_radians_reference(
    long double degrees)
{
    return degrees * PI_L / 180.0L;
}

static double microphone_position(int microphone)
{
    return
        (static_cast<double>(microphone) -
         static_cast<double>(MICROPHONE_COUNT - 1) / 2.0) *
        MICROPHONE_SPACING_METERS;
}

static void initialize_input()
{
    uint32_t state = 0x27182818u;

    const double desired_angle =
        degrees_to_radians(
            DESIRED_SOURCE_ANGLE_DEG);

    const double interferer_angle =
        degrees_to_radians(
            INTERFERER_SOURCE_ANGLE_DEG);

    for(int microphone = 0;
        microphone < MICROPHONE_COUNT;
        ++microphone) {

        const double position =
            microphone_position(microphone);

        const double desired_tau =
            position *
            std::sin(desired_angle) /
            SOUND_SPEED_METERS_PER_SECOND;

        const double interferer_tau =
            position *
            std::sin(interferer_angle) /
            SOUND_SPEED_METERS_PER_SECOND;

        for(int n = 0;
            n < INPUT_SAMPLES_PER_MIC;
            ++n) {

            state =
                1664525u * state +
                1013904223u;

            const uint32_t random24 =
                (state >> 8) & 0x00ffffffu;

            const double uniform01 =
                static_cast<double>(random24) /
                16777215.0;

            const double noise =
                NOISE_AMPLITUDE *
                (2.0 * uniform01 - 1.0);

            const double t =
                static_cast<double>(n) /
                static_cast<double>(SAMPLE_RATE);

            const double desired =
                DESIRED_SOURCE_AMPLITUDE *
                std::sin(
                    2.0 * PI *
                    DESIRED_SOURCE_FREQUENCY_HZ *
                    (t - desired_tau));

            const double interferer =
                INTERFERER_SOURCE_AMPLITUDE *
                std::sin(
                    2.0 * PI *
                    INTERFERER_SOURCE_FREQUENCY_HZ *
                    (t - interferer_tau));

            g_input[microphone][n] =
                desired +
                interferer +
                noise;
        }
    }
}

static void initialize_delay_table()
{
    for(int beam = 0;
        beam < BEAM_COUNT;
        ++beam) {

        const double angle_deg =
            BEAM_ANGLE_START_DEG +
            BEAM_ANGLE_STEP_DEG *
            static_cast<double>(beam);

        const double angle_rad =
            degrees_to_radians(angle_deg);

        for(int microphone = 0;
            microphone < MICROPHONE_COUNT;
            ++microphone) {

            const double position =
                microphone_position(microphone);

            const double tau =
                position *
                std::sin(angle_rad) /
                SOUND_SPEED_METERS_PER_SECOND;

            g_delay_samples[beam][microphone] =
                tau *
                static_cast<double>(SAMPLE_RATE);
        }
    }
}

static double production_sample_zero_padded(
    int microphone,
    int index)
{
    if(index < 0 ||
       index >= INPUT_SAMPLES_PER_MIC) {
        return 0.0;
    }

    return g_input[microphone][index];
}

static double production_interpolate(
    int microphone,
    double index)
{
    const double floored =
        std::floor(index);

    const int i0 =
        static_cast<int>(floored);

    const double fraction =
        index - floored;

    const double s0 =
        production_sample_zero_padded(
            microphone,
            i0);

    const double s1 =
        production_sample_zero_padded(
            microphone,
            i0 + 1);

    return
        (1.0 - fraction) * s0 +
        fraction * s1;
}

static void compute_one_beamform(
    double* output)
{
    for(int beam = 0;
        beam < BEAM_COUNT;
        ++beam) {

        for(int n = 0;
            n < OUTPUT_SAMPLES_PER_BEAM;
            ++n) {

            double sum = 0.0;

            for(int microphone = 0;
                microphone < MICROPHONE_COUNT;
                ++microphone) {

                const double index =
                    static_cast<double>(n) +
                    g_delay_samples[beam][microphone];

                sum +=
                    production_interpolate(
                        microphone,
                        index);
            }

            output[
                beam * OUTPUT_SAMPLES_PER_BEAM + n
            ] =
                sum /
                static_cast<double>(
                    MICROPHONE_COUNT);
        }
    }
}

static long double reference_sample_zero_padded(
    int microphone,
    int index)
{
    if(index < 0 ||
       index >= INPUT_SAMPLES_PER_MIC) {
        return 0.0L;
    }

    return
        static_cast<long double>(
            g_input[microphone][index]);
}

static long double reference_interpolate(
    int microphone,
    long double index)
{
    const long double floored =
        std::floor(index);

    const int i0 =
        static_cast<int>(floored);

    const long double fraction =
        index - floored;

    const long double s0 =
        reference_sample_zero_padded(
            microphone,
            i0);

    const long double s1 =
        reference_sample_zero_padded(
            microphone,
            i0 + 1);

    return
        (1.0L - fraction) * s0 +
        fraction * s1;
}

static void compute_reference_beamform(
    long double* output)
{
    for(int beam = 0;
        beam < BEAM_COUNT;
        ++beam) {

        const long double angle_deg =
            static_cast<long double>(
                BEAM_ANGLE_START_DEG) +
            static_cast<long double>(
                BEAM_ANGLE_STEP_DEG) *
            static_cast<long double>(beam);

        const long double angle_rad =
            degrees_to_radians_reference(
                angle_deg);

        long double local_shift[
            MICROPHONE_COUNT
        ];

        for(int microphone = 0;
            microphone < MICROPHONE_COUNT;
            ++microphone) {

            const long double position =
                (
                    static_cast<long double>(
                        microphone) -
                    static_cast<long double>(
                        MICROPHONE_COUNT - 1) /
                        2.0L
                ) *
                static_cast<long double>(
                    MICROPHONE_SPACING_METERS);

            const long double tau =
                position *
                std::sin(angle_rad) /
                static_cast<long double>(
                    SOUND_SPEED_METERS_PER_SECOND);

            local_shift[microphone] =
                tau *
                static_cast<long double>(
                    SAMPLE_RATE);
        }

        for(int n = 0;
            n < OUTPUT_SAMPLES_PER_BEAM;
            ++n) {

            long double sum = 0.0L;

            for(int microphone = 0;
                microphone < MICROPHONE_COUNT;
                ++microphone) {

                const long double index =
                    static_cast<long double>(n) +
                    local_shift[microphone];

                sum +=
                    reference_interpolate(
                        microphone,
                        index);
            }

            output[
                beam * OUTPUT_SAMPLES_PER_BEAM + n
            ] =
                sum /
                static_cast<long double>(
                    MICROPHONE_COUNT);
        }
    }
}

int main()
{
    initialize_input();
    initialize_delay_table();

    roi_begin();

    for(int run = 0;
        run < BEAMFORM_RUNS;
        ++run) {

        compute_one_beamform(
            g_output);
    }

    roi_end();

    compute_reference_beamform(
        g_reference);

    bool finite_pass = true;

    long double max_abs_error = 0.0L;
    long double squared_error_sum = 0.0L;
    long double checksum = 0.0L;

    for(int i = 0;
        i < OUTPUT_VALUES;
        ++i) {

        const long double production =
            static_cast<long double>(
                g_output[i]);

        const long double reference =
            g_reference[i];

        if(!std::isfinite(production) ||
           !std::isfinite(reference)) {
            finite_pass = false;
        }

        const long double difference =
            production -
            reference;

        const long double absolute_error =
            std::fabs(difference);

        if(absolute_error >
           max_abs_error) {
            max_abs_error =
                absolute_error;
        }

        squared_error_sum +=
            difference *
            difference;

        checksum +=
            production *
            static_cast<long double>(
                i + 1);
    }

    const long double rmse =
        std::sqrt(
            squared_error_sum /
            static_cast<long double>(
                OUTPUT_VALUES));

    const bool oracle_pass =
        finite_pass &&
        max_abs_error <= 1.0e-9L &&
        rmse <= 1.0e-10L;

    std::printf(
        "CP25_WORKLOAD=DELAY_SUM_BEAMFORMING\n");

    std::printf(
        "BEAMFORM_RUNS=%d\n",
        BEAMFORM_RUNS);

    std::printf(
        "MICROPHONE_COUNT=%d\n",
        MICROPHONE_COUNT);

    std::printf(
        "INPUT_SAMPLES_PER_MIC=%d\n",
        INPUT_SAMPLES_PER_MIC);

    std::printf(
        "BEAM_COUNT=%d\n",
        BEAM_COUNT);

    std::printf(
        "OUTPUT_VALUES=%d\n",
        OUTPUT_VALUES);

    std::printf(
        "FINITE=%s\n",
        finite_pass ? "PASS" : "FAIL");

    std::printf(
        "MAX_ABS_ERROR=%.17Lg\n",
        max_abs_error);

    std::printf(
        "RMSE=%.17Lg\n",
        rmse);

    std::printf(
        "CHECKSUM=%.17Lg\n",
        checksum);

    std::printf(
        "ORACLE=%s\n",
        oracle_pass ? "PASS" : "FAIL");

    if(!oracle_pass)
        return 2;

    std::printf(
        "CP25_BEAMFORMING_NATIVE_OK\n");

    return 0;
}