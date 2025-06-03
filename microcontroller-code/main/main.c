#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lcd/lcd.h"
#include "input/input.h"
#include "ble/ble.h"

#define TAG "MAIN"

static int point_x = 0;
static int point_y = 0;
static int point2_x = 0;
static int point2_y = 0;

static void update_point_position(int joystick_x, int joystick_y,
                                  int square_x, int square_y,
                                  int square_size, bool invert_x,
                                  int *px, int *py)
{
    const int max_val = 4095;
    *py = square_y + ((joystick_y * square_size) / max_val) - (square_size / 2);

    if (invert_x) {
        *px = square_x + ((max_val - joystick_x) * square_size) / max_val - (square_size / 2);
    } else {
        *px = square_x + (joystick_x * square_size) / max_val - (square_size / 2);
    }

    *px = (*px < 0) ? 0 : (*px >= LCD_WIDTH) ? LCD_WIDTH - 1
                                             : *px;
    *py = (*py < 0) ? 0 : (*py >= LCD_HEIGHT) ? LCD_HEIGHT - 1
                                              : *py;
}

void app_main() {
    // LCD
    lcd_init();
    lcd_clear_screen(0x0000);

    // Static text
    lcd_draw_text_fast("X:", 14, 15, 0x06db, 0x0000);
    lcd_draw_text_fast("Y:", 14, 25, 0x06db, 0x0000);
    lcd_draw_text_fast("SW:", 5, 35, 0x06db, 0x0000);
    lcd_draw_text_fast("X:", 100, 15, 0x07E0, 0x0000);
    lcd_draw_text_fast("Y:", 100, 25, 0x07E0, 0x0000);

    // Init input
    input_init();

    // init ble
    ble_init();

    // square
    const int square_size = 50;
    lcd_draw_square_fast(35, 75, square_size, 0xFFFF);
    lcd_draw_square_fast(120, 75, square_size, 0xFFFF);

    // buf for vals
    char buf_x[16], buf_y[16], buf_sw[16], buf_x2[16], buf_y2[16];
    char old_buf_x[16] = {0}, old_buf_y[16] = {0},
         old_buf_sw[16] = {0}, old_x2[16] = {0}, old_y2[16] = {0};

    // main in main :)
    while (1) {
        // read from Joystick
        JoystickData data = input_read();

        // update BLE
        ble_update_joystick_data(data.x1, data.y1, data.x2, data.y2, data.sw1);

        // clear
        snprintf(buf_x, sizeof(buf_x), "%d", data.x1);
        snprintf(buf_y, sizeof(buf_y), "%d", data.y1);
        snprintf(buf_sw, sizeof(buf_sw), "%c", (data.sw1 == 0) ? '+' : '-');
        snprintf(buf_x2, sizeof(buf_x2), "%d", data.x2);
        snprintf(buf_y2, sizeof(buf_y2), "%d", data.y2);

        // update text
        if (strcmp(buf_x, old_buf_x) != 0) {
            lcd_draw_text_fast(old_buf_x, 30, 15, 0x0000, 0x0000);
            lcd_draw_text_fast(buf_x, 30, 15, 0xffff, 0x0000);
            strcpy(old_buf_x, buf_x);
        }
        if (strcmp(buf_y, old_buf_y) != 0) {
            lcd_draw_text_fast(old_buf_y, 30, 25, 0x0000, 0x0000);
            lcd_draw_text_fast(buf_y, 30, 25, 0xffff, 0x0000);
            strcpy(old_buf_y, buf_y);
        }
        if (strcmp(buf_sw, old_buf_sw) != 0) {
            lcd_draw_text_fast(old_buf_sw, 30, 35, 0x0000, 0x0000);
            lcd_draw_text_fast(buf_sw, 30, 35, 0xffff, 0x0000);
            strcpy(old_buf_sw, buf_sw);
        }
        if (strcmp(buf_x2, old_x2) != 0) {
            lcd_draw_text_fast(old_x2, 120, 15, 0x0000, 0x0000);
            lcd_draw_text_fast(buf_x2, 120, 15, 0xffff, 0x0000);
            strcpy(old_x2, buf_x2);
        }
        if (strcmp(buf_y2, old_y2) != 0) {
            lcd_draw_text_fast(old_y2, 120, 25, 0x0000, 0x0000);
            lcd_draw_text_fast(buf_y2, 120, 25, 0xffff, 0x0000);
            strcpy(old_y2, buf_y2);
        }

        // update points
        lcd_draw_point_in_square(point_x, point_y, 0x0000); // clear
        update_point_position(data.x1, data.y1, 35, 75, square_size, true, &point_x, &point_y);
        uint16_t point_color = (data.sw1 == 0) ? 0xf80c : 0x06db;
        lcd_draw_point_in_square(point_x, point_y, point_color);

        lcd_draw_point_in_square(point2_x, point2_y, 0x0000); // clear
        update_point_position(data.x2, data.y2, 120, 75, square_size, true, &point2_x, &point2_y);
        lcd_draw_point_in_square(point2_x, point2_y, 0x07E0);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}