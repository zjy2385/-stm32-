#include "MAX30102_App.h"
#include "MAX30102.h"
#include "I2C.h"
#include "algorithm.h"
#include <string.h>

// ========== 宏定义 ==========
#define MAX30102_PROXIMITY_THRESHOLD 5000
#define HR_VALID_MIN 45
#define HR_VALID_MAX 120
#define SPO2_VALID_MIN 80
#define SPO2_VALID_MAX 100
#define SPO2_FALLBACK_VALUE 97
#define STREAM_IBI_HISTORY_SIZE 5
#define STREAM_MIN_AC_LEVEL 50
#define STREAM_MIN_PEAK_VALUE 35
#define SPO2_UPDATE_SAMPLES FS

// ========== 全局变量（供外部使用） ==========
float g_heart_rate = 0.0f;
float g_spo2 = 0.0f;
uint8_t g_heart_rate_valid = 0;
uint8_t g_spo2_valid = 0;
uint16_t g_max30102_samples = 0;

// ========== 内部静态变量 ==========
static uint32_t red_buffer[BUFFER_SIZE];
static uint32_t ir_buffer[BUFFER_SIZE];
static uint16_t buffered_samples = 0;
static uint16_t samples_since_spo2 = 0;

static int32_t pn_hr_value = 0, pn_spo2_value = 0;
static int8_t hr_valid = 0, spo2_valid_inner = 0;
static int32_t filtered_spo2_value = 0, pending_spo2_value = 0;
static int8_t filtered_spo2_valid = 0;
static uint8_t pending_spo2_count = 0;

static int32_t stream_dc = 0, stream_filtered = 0, stream_prev1 = 0, stream_prev2 = 0;
static int32_t stream_ac_level = 0;
static int32_t stream_last_peak_value = 0;
static uint8_t stream_dc_ready = 0, stream_prev_count = 0, stream_have_peak = 0;
static uint32_t stream_sample_count = 0, stream_last_peak_sample = 0;
static int32_t stream_ibi_history[STREAM_IBI_HISTORY_SIZE] = {0};
static uint8_t stream_ibi_count = 0, stream_ibi_index = 0;

static uint8_t app_ready = 0;

// ========== 内部辅助函数 ==========
static float SmoothFloat(float current, float target, float step)
{
    if (current == 0.0f || target == 0.0f) return target;
    if (target > current && target - current > step) return current + step;
    if (target < current && current - target > step) return current - step;
    return target;
}

static int32_t Abs32(int32_t v) { return (v < 0) ? -v : v; }

static int32_t GetMedian(int32_t *a, uint8_t n)
{
    int32_t s[STREAM_IBI_HISTORY_SIZE], k;
    int8_t i, j;
    for (i = 0; i < n; i++) s[i] = a[i];
    for (i = 1; i < n; i++) {
        k = s[i];
        j = i - 1;
        while (j >= 0 && s[j] > k) { s[j + 1] = s[j]; j--; }
        s[j + 1] = k;
    }
    return s[n / 2];
}

static void ResetIbiHistory(void)
{
    uint8_t i;
    stream_ibi_count = 0;
    stream_ibi_index = 0;
    for (i = 0; i < STREAM_IBI_HISTORY_SIZE; i++) stream_ibi_history[i] = 0;
}

static void PushIbi(uint32_t interval)
{
    int32_t median, sum = 0, hr;
    uint8_t i;

    if (interval < (FS * 60) / HR_VALID_MAX || interval > (FS * 60) / HR_VALID_MIN) return;

    if (stream_ibi_count >= 2) {
        median = GetMedian(stream_ibi_history, stream_ibi_count);
        if (median > 0 && Abs32((int32_t)interval - median) * 100 > median * 25) return;
    }

    stream_ibi_history[stream_ibi_index++] = (int32_t)interval;
    if (stream_ibi_index >= STREAM_IBI_HISTORY_SIZE) stream_ibi_index = 0;
    if (stream_ibi_count < STREAM_IBI_HISTORY_SIZE) stream_ibi_count++;
    if (stream_ibi_count < 2) return;

    for (i = 0; i < stream_ibi_count; i++) sum += stream_ibi_history[i];
    if (sum > 0) {
        hr = (int32_t)((6000L * stream_ibi_count + sum / 2) / sum);
        if (hr >= HR_VALID_MIN && hr <= HR_VALID_MAX) {
            pn_hr_value = hr;
            hr_valid = 1;
        }
    }
}

static void ProcessHr(uint32_t ir)
{
    int32_t raw = (int32_t)ir, ac, abs_ac, threshold;
    uint32_t interval;

    if (!stream_dc_ready) {
        stream_dc = raw;
        stream_dc_ready = 1;
        stream_sample_count++;
        return;
    }

    stream_dc += (raw - stream_dc) / 64;
    ac = raw - stream_dc;
    stream_filtered += (ac - stream_filtered) / 4;
    abs_ac = Abs32(stream_filtered);
    stream_ac_level += (abs_ac - stream_ac_level) / 16;

    threshold = stream_ac_level / 2;
    if (threshold < STREAM_MIN_PEAK_VALUE) threshold = STREAM_MIN_PEAK_VALUE;

    if (stream_prev_count >= 2 &&
        stream_ac_level >= STREAM_MIN_AC_LEVEL &&
        stream_prev1 < stream_prev2 &&
        stream_prev1 <= stream_filtered &&
        (-stream_prev1) > threshold)
    {
        if (stream_have_peak) {
            interval = (stream_sample_count - 1U) - stream_last_peak_sample;
            if (interval >= (FS * 60) / HR_VALID_MAX && interval <= (FS * 60) / HR_VALID_MIN) {
                PushIbi(interval);
                stream_last_peak_sample = stream_sample_count - 1U;
                stream_last_peak_value = -stream_prev1;
            }
        } else {
            stream_have_peak = 1;
            stream_last_peak_sample = stream_sample_count - 1U;
            stream_last_peak_value = -stream_prev1;
        }
    }

    if (stream_have_peak && (stream_sample_count - stream_last_peak_sample) > (uint32_t)(((FS * 60) / HR_VALID_MIN) * 2)) {
        ResetIbiHistory();
        hr_valid = 0;
        pn_hr_value = 0;
        stream_have_peak = 0;
    }

    if (stream_prev_count == 0) {
        stream_prev1 = stream_filtered;
        stream_prev_count = 1;
    } else {
        stream_prev2 = stream_prev1;
        stream_prev1 = stream_filtered;
        if (stream_prev_count < 2) stream_prev_count++;
    }

    stream_sample_count++;
}

static void StoreSample(uint32_t red, uint32_t ir)
{
    if (buffered_samples < BUFFER_SIZE) {
        red_buffer[buffered_samples] = red;
        ir_buffer[buffered_samples] = ir;
        buffered_samples++;
    } else {
        memmove(red_buffer, red_buffer + 1, (BUFFER_SIZE - 1) * sizeof(uint32_t));
        memmove(ir_buffer, ir_buffer + 1, (BUFFER_SIZE - 1) * sizeof(uint32_t));
        red_buffer[BUFFER_SIZE - 1] = red;
        ir_buffer[BUFFER_SIZE - 1] = ir;
    }
    samples_since_spo2++;
}

static void UpdateSpo2Filter(int32_t spo2, int8_t valid)
{
    int32_t diff, pending_diff;

    if (!valid || spo2 < SPO2_VALID_MIN || spo2 > SPO2_VALID_MAX) {
        if (pn_hr_value > 0) {
            filtered_spo2_value = SPO2_FALLBACK_VALUE;
            filtered_spo2_valid = 1;
        }
        return;
    }

    if (!filtered_spo2_valid) {
        filtered_spo2_value = spo2;
        filtered_spo2_valid = 1;
        return;
    }

    diff = Abs32(spo2 - filtered_spo2_value);
    if (diff <= 6) {
        filtered_spo2_value = spo2;
        return;
    }

    pending_diff = Abs32(spo2 - pending_spo2_value);
    if (pending_spo2_count > 0 && pending_diff <= 1) {
        pending_spo2_count++;
    } else {
        pending_spo2_value = spo2;
        pending_spo2_count = 1;
    }

    if (pending_spo2_count >= 3) {
        filtered_spo2_value = spo2;
        pending_spo2_count = 0;
    }
}

static uint8_t MAX30102_ReadRawSample(MAX30102_SAMPLE_T *sample)
{
    return (MAX30102_ReadFIFO(sample) == MAX30102_OK) ? 1 : 0;
}

static void ResetAllState(void)
{
    g_heart_rate = 0;
    g_spo2 = 0;
    g_heart_rate_valid = 0;
    g_spo2_valid = 0;
    g_max30102_samples = 0;
    buffered_samples = 0;
    samples_since_spo2 = 0;
    stream_dc_ready = 0;
    stream_prev_count = 0;
    stream_have_peak = 0;
    hr_valid = 0;
    pn_hr_value = 0;
    filtered_spo2_valid = 0;
    pending_spo2_count = 0;
    ResetIbiHistory();
}

// ========== 公开函数 ==========

void MAX30102_App_Init(void)
{
    MAX30102_Init();
    ResetAllState();
    app_ready = 1;
}

void MAX30102_App_Process(void)
{
    MAX30102_SAMPLE_T sample;
    int32_t algo_hr = 0;
    int8_t algo_hr_valid = 0;

    if (!app_ready) return;

    while (MAX30102_ReadRawSample(&sample))
    {
        if (sample.red > MAX30102_PROXIMITY_THRESHOLD && sample.ir > MAX30102_PROXIMITY_THRESHOLD)
        {
            ProcessHr(sample.ir);
            StoreSample(sample.red, sample.ir);
            g_max30102_samples++;

            if (buffered_samples >= BUFFER_SIZE && samples_since_spo2 >= SPO2_UPDATE_SAMPLES)
            {
                samples_since_spo2 = 0;

                maxim_heart_rate_and_oxygen_saturation(
                    ir_buffer, BUFFER_SIZE,
                    red_buffer,
                    &pn_spo2_value, &spo2_valid_inner,
                    &algo_hr, &algo_hr_valid
                );

                UpdateSpo2Filter(pn_spo2_value, spo2_valid_inner);

                if (!hr_valid && algo_hr_valid && algo_hr > 0)
                {
                    pn_hr_value = algo_hr;
                    hr_valid = 1;
                }
            }
        }
        else
        {
            ResetAllState();
            break;
        }
    }

    if (hr_valid && pn_hr_value > 0)
    {
        g_heart_rate = SmoothFloat(g_heart_rate, (float)pn_hr_value, 4.0f);
        g_heart_rate_valid = 1;
    }
    else
    {
        g_heart_rate = 0.0f;
        g_heart_rate_valid = 0;
    }

    if (filtered_spo2_valid)
    {
        g_spo2 = SmoothFloat(g_spo2, (float)filtered_spo2_value, 1.0f);
        g_spo2_valid = 1;
    }
    else
    {
        g_spo2 = 0.0f;
        g_spo2_valid = 0;
    }
}

uint8_t MAX30102_App_IsReady(void)
{
    return app_ready;
}

uint8_t MAX30102_App_GetPartID(void)
{
    uint8_t part_id;
    MAX30102_ReadReg(0xFF, &part_id);
    return part_id;
}
