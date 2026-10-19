#include <stdio.h>
#include "pico/stdlib.h"

#define MAX_TEMP 8500 // Encoded for the rounded numbers
#define MIN_TEMP -4000 // Encoded for the rounded numbers

float setpoint = 25.0;

void setup() {
    stdio_init_all();
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}

// If anything is obtained through the USB communication, interpret that as the setpoint temperature
void set_temperature(int c) {
    setpoint = 0;
    bool is_positive = true;
    if (c == '-') is_positive = false;

    while (c != '\n') {
        // Handle error, PC should try again
        if (c == PICO_ERROR_TIMEOUT) {
            printf("NOK\n");
            return;
        };

        if (c >= '0' && c <= '9') setpoint = setpoint*10 + (c - '0');

        if ((is_positive && setpoint > MAX_TEMP) || (!is_positive && -1*setpoint < MIN_TEMP)) {
            printf("NOK\n");
            return;
        }
        c = getchar_timeout_us(500);
    }
    setpoint /= 100;
    setpoint *= is_positive ? 1 : -1;
    printf("OK %f\n", setpoint);
}

// When we have the sensors up and running, this will be changed to the 
// actual sensor readings. Right now, they are just fluctuating up and down
void dummy_readings() {
    static bool temp_rise = true;
    static bool hum_rise = false;
    static float temp = 25.0;
    static float hum = 40.0;

    /* temp_rise true when temp  <= -40 
     * temp_rise false when temp >= 85
     * else temp_rise = temp_rise */
    if (temp <= -40.0)
        temp_rise = true;
    else if (temp >= 85.0)
        temp_rise = false;
    temp = temp_rise ? temp+0.5 : temp-0.5;

    if (hum <= 0.0)
        hum_rise = true;
    else if (hum >= 100.0)
        hum_rise = false;
    hum = hum_rise ? hum+0.5 : hum-0.5;

    printf("DATA:%f,%f\n", temp, hum);
}


int main() {
    setup();
    int i = 0;
    
    while (true) {
        int c = getchar_timeout_us(0);  // non-blocking
        if (c != PICO_ERROR_TIMEOUT) {
            set_temperature(c);
        }

        // LED toggles any time there is any reading, to show that it is working
        gpio_put(PICO_DEFAULT_LED_PIN, (i++)%2 == 0);

        dummy_readings();

        sleep_ms(500);
    }
}
