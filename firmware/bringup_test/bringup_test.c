/*
 * CTEC bring-up test (Pico 2)
 *
 * 1. Reads the YL-38 thermistor module:
 *      AO -> GP26 (ADC0), DO -> GP21, VCC -> 3V3(OUT), GND -> GND
 * 2. Drives the TEC control pins exactly as in the CTEC schematic:
 *      TEC_PWM -> GP14 (20 kHz), TEC_DIR -> GP15
 *    and sweeps the duty cycle 0 -> 25 -> 50 -> 75 -> 100 % -> 0,
 *    flipping the direction after every sweep.
 * 3. FAN_PWM -> GP12 (25 kHz), fixed at 100 %.
 *
 * !!! TEST WITH LEDs ONLY. Do NOT connect the H-bridge / Peltier while this
 * !!! program runs: it switches direction and goes to 100 % on its own.
 *
 * Output: one line per second over USB serial (115200), e.g.
 *   t=12s adc=2047 R=10002 T=24.99C DO=1 | TEC dir=COOL duty=50%
 */

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

/* ---- Pin map (from CTEC.kicad_sch) ---- */
#define PIN_TEC_PWM   14
#define PIN_TEC_DIR   15
#define PIN_FAN_PWM   12
#define PIN_NTC_AO    26   /* ADC input 0 */
#define PIN_NTC_DO    21
#define ADC_CH_NTC    0

/* ---- YL-38 thermistor (assumed values, not confirmed) ---- */
#define R_FIXED       10000.0f  /* R1 on the module, VCC side */
#define NTC_R25       10000.0f
#define NTC_BETA      3950.0f
#define ADC_MAX       4095.0f   /* 12-bit */

#define TEC_PWM_HZ    20000u
#define FAN_PWM_HZ    25000u

static uint tec_slice, fan_slice;
static uint16_t tec_wrap, fan_wrap;

/* Set up one PWM pin at the given frequency; returns the wrap value. */
static uint16_t pwm_setup(uint pin, uint32_t freq_hz, uint *slice_out) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    uint32_t wrap = clock_get_hz(clk_sys) / freq_hz - 1;  /* 150 MHz / 20 kHz -> 7499 */
    pwm_set_clkdiv(slice, 1.0f);
    pwm_set_wrap(slice, (uint16_t)wrap);
    pwm_set_gpio_level(pin, 0);           /* start OFF */
    pwm_set_enabled(slice, true);
    *slice_out = slice;
    return (uint16_t)wrap;
}

static void set_duty(uint pin, uint16_t wrap, float pct) {
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    pwm_set_gpio_level(pin, (uint16_t)(pct / 100.0f * (wrap + 1)));
}

/* Direction change is only allowed with zero output: PWM 0 -> wait -> DIR -> wait */
static void tec_set_dir(bool heat) {
    set_duty(PIN_TEC_PWM, tec_wrap, 0.0f);
    sleep_ms(10);
    gpio_put(PIN_TEC_DIR, heat);
    sleep_ms(10);
}

/* Average 16 ADC samples to reduce noise. */
static float read_ntc_adc(void) {
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += adc_read();
    return sum / 16.0f;
}

/* AO = VCC * Rntc / (Rfixed + Rntc)  ->  Rntc = Rfixed * n / (nmax - n) */
static float adc_to_resistance(float n) {
    if (n <= 0.5f) return 0.0f;                 /* short to GND */
    if (n >= ADC_MAX - 0.5f) return INFINITY;   /* open / disconnected */
    return R_FIXED * n / (ADC_MAX - n);
}

static float resistance_to_celsius(float r) {
    if (r <= 0.0f || isinf(r)) return NAN;
    float inv_t = 1.0f / 298.15f + logf(r / NTC_R25) / NTC_BETA;
    return 1.0f / inv_t - 273.15f;
}

int main(void) {
    stdio_init_all();

    /* Outputs first, so the TEC pin is driven low as early as possible. */
    tec_wrap = pwm_setup(PIN_TEC_PWM, TEC_PWM_HZ, &tec_slice);
    gpio_init(PIN_TEC_DIR);
    gpio_set_dir(PIN_TEC_DIR, GPIO_OUT);
    gpio_put(PIN_TEC_DIR, 0);

    fan_wrap = pwm_setup(PIN_FAN_PWM, FAN_PWM_HZ, &fan_slice);
    set_duty(PIN_FAN_PWM, fan_wrap, 100.0f);    /* fan always on */

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    /* DO has a 10k pull-up on the module: plain input, no internal pulls. */
    gpio_init(PIN_NTC_DO);
    gpio_set_dir(PIN_NTC_DO, GPIO_IN);
    gpio_disable_pulls(PIN_NTC_DO);

    adc_init();
    adc_gpio_init(PIN_NTC_AO);
    adc_select_input(ADC_CH_NTC);

    sleep_ms(2000);   /* give time to open the serial monitor */
    printf("\nCTEC bring-up test. sys clk = %lu Hz, TEC wrap = %u\n",
           (unsigned long)clock_get_hz(clk_sys), tec_wrap);

    const float steps[] = {0, 25, 50, 75, 100, 0};
    const int n_steps = sizeof(steps) / sizeof(steps[0]);
    int step = 0;
    bool heat = false;
    uint32_t t = 0;

    while (true) {
        /* ---- TEC output: one duty step every 2 s ---- */
        if (t % 2 == 0) {
            if (step == 0) {          /* new sweep: flip direction safely */
                heat = !heat;
                tec_set_dir(heat);
            }
            set_duty(PIN_TEC_PWM, tec_wrap, steps[step]);
            step = (step + 1) % n_steps;
        }
        int shown = (step + n_steps - 1) % n_steps;

        /* ---- Thermistor ---- */
        float n = read_ntc_adc();
        float r = adc_to_resistance(n);
        float temp = resistance_to_celsius(r);
        int dout = gpio_get(PIN_NTC_DO);

        if (isnan(temp)) {
            printf("t=%lus adc=%.0f  SENSOR FAULT (%s) DO=%d | TEC dir=%s duty=%.0f%%\n",
                   (unsigned long)t, n, n < 1 ? "short" : "open", dout,
                   heat ? "HEAT" : "COOL", steps[shown]);
        } else {
            printf("t=%lus adc=%.0f R=%.0f T=%.2fC DO=%d | TEC dir=%s duty=%.0f%%\n",
                   (unsigned long)t, n, r, temp, dout,
                   heat ? "HEAT" : "COOL", steps[shown]);
        }

        gpio_put(PICO_DEFAULT_LED_PIN, t % 2);  /* heartbeat */
        t++;
        sleep_ms(1000);
    }
}
