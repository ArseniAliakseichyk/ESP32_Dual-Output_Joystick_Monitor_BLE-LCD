#include "input.h"
#include "driver/adc.h"
#include "driver/gpio.h"

#define X_1 0
#define Y_1 1
#define SW_1 9
#define X_2 3
#define Y_2 2

void input_init(void) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(X_1, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(Y_1, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(X_2, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(Y_2, ADC_ATTEN_DB_11);
    
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SW_1),
        .mode = GPIO_MODE_INPUT,
    };
    gpio_config(&io_conf);
    gpio_pullup_en(SW_1);
}

JoystickData input_read(void) {
    return (JoystickData){
        .x1 = adc1_get_raw(X_1),
        .y1 = adc1_get_raw(Y_1),
        .x2 = adc1_get_raw(X_2),
        .y2 = adc1_get_raw(Y_2),
        .sw1 = gpio_get_level(SW_1)
    };
}