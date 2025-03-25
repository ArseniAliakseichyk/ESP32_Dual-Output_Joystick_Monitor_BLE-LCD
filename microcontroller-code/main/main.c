#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#define TAG "BLE_SIMPLE"
#define DEVICE_NAME "BLE-Arseni"

#define PIN_NUM_MOSI 4
#define PIN_NUM_CLK  5
#define PIN_NUM_CS   6
#define PIN_NUM_DC   7
#define PIN_NUM_RST  8

#define X_1 0 // GP0
#define Y_1 1 // GP1
#define SW_1 9 // GP9

#define X_2 3 // GP3
#define Y_2 2 // GP2

#define LCD_WIDTH  160
#define LCD_HEIGHT 128

spi_device_handle_t spi;
static uint8_t ble_addr_type;

static int last_x1 = 0;
static int last_y1 = 0;
static int last_x2 = 0;
static int last_y2 = 0;
static int last_sw1 = 0;

static void ble_app_advertise(void);

// Мини-шрифт 8x8
const uint8_t font8x8_basic[128][8] = {
    ['H'] = {0b10000010, 0b10000010, 0b10000010, 0b11111110, 0b10000010, 0b10000010, 0b10000010, 0b00000000},
    ['E'] = {0b11111110, 0b10000000, 0b10000000, 0b11111110, 0b10000000, 0b10000000, 0b11111110, 0b00000000},
    ['L'] = {0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b11111110, 0b00000000},
    ['O'] = {0b01111100, 0b10000010, 0b10000010, 0b10000010, 0b10000010, 0b10000010, 0b01111100, 0b00000000},
    [' '] = {0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000},
    ['W'] = {0b10000010, 0b10000010, 0b10000010, 0b10000010, 0b10010010, 0b10101010, 0b11000110, 0b00000000},
    ['R'] = {0b11111100, 0b10000010, 0b10000010, 0b11111100, 0b10001000, 0b10000100, 0b10000010, 0b00000000},
    ['D'] = {0b11111100, 0b10000010, 0b10000010, 0b10000010, 0b10000010, 0b10000010, 0b11111100, 0b00000000},
    ['*'] = {0b00001000, 0b00000100, 0b11100010, 0b00000010, 0b11100010, 0b00000100, 0b00001000, 0b00000000},
    ['-'] = {0b00000000, 0b00000000, 0b00000000, 0b01111100, 0b00000000, 0b00000000, 0b00000000, 0b00000000},
    ['0'] = {0b01111100, 0b10000110, 0b10001010, 0b10010010, 0b10100010, 0b11000010, 0b01111100, 0b00000000},
    ['1'] = {0b00010000, 0b00110000, 0b01010000, 0b00010000, 0b00010000, 0b00010000, 0b01111100, 0b00000000},
    ['2'] = {0b01111100, 0b10000010, 0b00000010, 0b00011100, 0b01100000, 0b10000000, 0b11111110, 0b00000000},
    ['3'] = {0b11111110, 0b00000010, 0b00000100, 0b00011100, 0b00000010, 0b10000010, 0b01111100, 0b00000000},
    ['4'] = {0b00001100, 0b00110100, 0b01000100, 0b10000100, 0b11111110, 0b00000100, 0b00000100, 0b00000000},
    ['5'] = {0b11111110, 0b10000000, 0b11111100, 0b00000010, 0b00000010, 0b10000010, 0b01111100, 0b00000000},
    ['6'] = {0b00111100, 0b01000000, 0b10000000, 0b11111100, 0b10000010, 0b10000010, 0b01111100, 0b00000000},
    ['7'] = {0b11111110, 0b00000010, 0b00000100, 0b00001000, 0b00010000, 0b00100000, 0b01000000, 0b00000000},
    ['8'] = {0b01111100, 0b10000010, 0b10000010, 0b01111100, 0b10000010, 0b10000010, 0b01111100, 0b00000000},
    ['9'] = {0b01111100, 0b10000010, 0b10000010, 0b01111110, 0b00000010, 0b00000100, 0b01111000, 0b00000000},
    ['X'] = {0b10000010, 0b01000100, 0b00101000, 0b00010000, 0b00101000, 0b01000100, 0b10000010, 0b00000000},
    ['Y'] = {0b10000010, 0b01000100, 0b00101000, 0b00010000, 0b00010000, 0b00010000, 0b00010000, 0b00000000},
    ['+'] = {0b00010000, 0b00010000, 0b00010000, 0b11111110, 0b00010000, 0b00010000, 0b00010000, 0b00000000},
    [':'] = {0b00000000, 0b00000000, 0b00010000, 0b00000000, 0b00010000, 0b00000000, 0b00000000, 0b00000000},
    ['S'] = {0b00111100, 0b01000010, 0b10000000, 0b01111100, 0b00000010, 0b10000010, 0b01111100, 0b00000000}
};

void lcd_send_cmd(uint8_t cmd) {
    gpio_set_level(PIN_NUM_DC, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd
    };
    spi_device_polling_transmit(spi, &t);
}

void lcd_send_data(uint8_t data) {
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data
    };
    spi_device_polling_transmit(spi, &t);
}

void lcd_reset() {
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
}

void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    lcd_send_cmd(0x2A);
    lcd_send_data(0x00); lcd_send_data(x0);
    lcd_send_data(0x00); lcd_send_data(x1);

    lcd_send_cmd(0x2B);
    lcd_send_data(0x00); lcd_send_data(y0);
    lcd_send_data(0x00); lcd_send_data(y1);

    lcd_send_cmd(0x2C);
}

void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    lcd_set_window(x, y, x, y);
    gpio_set_level(PIN_NUM_DC, 1);

    uint8_t data[] = { color >> 8, color & 0xFF };
    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = data
    };
    spi_device_polling_transmit(spi, &t);
}

void lcd_clear_screen(uint16_t color) {
    lcd_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    gpio_set_level(PIN_NUM_DC, 1);
    uint8_t line[LCD_WIDTH * 2];
    for (int i = 0; i < LCD_WIDTH; i++) {
        line[i * 2] = color >> 8;
        line[i * 2 + 1] = color & 0xFF;
    }
    for (int y = 0; y < LCD_HEIGHT; y++) {
        spi_transaction_t t = {
            .length = LCD_WIDTH * 2 * 8,
            .tx_buffer = line
        };
        spi_device_polling_transmit(spi, &t);
    }
}
void lcd_init() {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST),
    };
    gpio_config(&io_conf);

    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_CLK,
        .max_transfer_sz = 4096,
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 15 * 1000 * 1000, // Up to 15 МГц
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi);

    lcd_reset();
    lcd_send_cmd(0x11); vTaskDelay(pdMS_TO_TICKS(120));
    lcd_send_cmd(0x3A); lcd_send_data(0x05); // 16-bit color
    lcd_send_cmd(0x36); lcd_send_data(0x60);
    lcd_send_cmd(0x29); // Display ON
}

void lcd_draw_char_fast(uint8_t ch, int x, int y, uint16_t fg_color, uint16_t bg_color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + 7 >= LCD_WIDTH || y + 7 >= LCD_HEIGHT) return;
    const uint8_t *glyph = font8x8_basic[ch];
    uint8_t buffer[64 * 2];
    int index = 0;
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            uint16_t color = (bits & (1 << (7 - col))) ? fg_color : bg_color;
            buffer[index++] = color >> 8;
            buffer[index++] = color & 0xFF;
        }
    }
    lcd_set_window(x, y, x + 7, y + 7);
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = 128 * 8,
        .tx_buffer = buffer
    };
    spi_device_polling_transmit(spi, &t);
}

void lcd_draw_text_fast(const char *text, int x, int y, uint16_t fg_color, uint16_t bg_color) {
    int current_x = x;
    while (*text && current_x < LCD_WIDTH) {
        lcd_draw_char_fast(*text, current_x, y, fg_color, bg_color);
        current_x += 9;
        text++;
    }
}

void lcd_draw_2x2(int x, int y, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    if (x + 1 >= LCD_WIDTH || y + 1 >= LCD_HEIGHT) return;
    uint8_t buffer[4 * 2];
    for (int i = 0; i < 4; i++) {
        buffer[i * 2] = color >> 8;
        buffer[i * 2 + 1] = color & 0xFF;
    }
    lcd_set_window(x, y, x + 1, y + 1);
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = 8 * 8,
        .tx_buffer = buffer
    };
    spi_device_polling_transmit(spi, &t);
}

void lcd_draw_hline_fast(int x_start, int x_end, int y, uint16_t color) {
    if (y < 0 || y >= LCD_HEIGHT) return;
    int draw_x_start = x_start < 0 ? 0 : x_start;
    int draw_x_end = x_end > LCD_WIDTH - 1 ? LCD_WIDTH - 1 : x_end;
    if (draw_x_start > draw_x_end) return;
    int width = draw_x_end - draw_x_start + 1;
    uint8_t buffer[width * 2];
    for (int i = 0; i < width; i++) {
        buffer[i * 2] = color >> 8;
        buffer[i * 2 + 1] = color & 0xFF;
    }
    lcd_set_window(draw_x_start, y, draw_x_end, y);
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = width * 16,
        .tx_buffer = buffer
    };
    spi_device_polling_transmit(spi, &t);
}

void lcd_draw_vline_fast(int y_start, int y_end, int x, uint16_t color) {
    if (x < 0 || x >= LCD_WIDTH) return;
    int draw_y_start = y_start < 0 ? 0 : y_start;
    int draw_y_end = y_end > LCD_HEIGHT - 1 ? LCD_HEIGHT - 1 : y_end;
    if (draw_y_start > draw_y_end) return;
    int height = draw_y_end - draw_y_start + 1;
    uint8_t buffer[height * 2];
    for (int i = 0; i < height; i++) {
        buffer[i * 2] = color >> 8;
        buffer[i * 2 + 1] = color & 0xFF;
    }
    lcd_set_window(x, draw_y_start, x, draw_y_end);
    gpio_set_level(PIN_NUM_DC, 1);
    spi_transaction_t t = {
        .length = height * 16,
        .tx_buffer = buffer
    };
    spi_device_polling_transmit(spi, &t);
}

void lcd_draw_square_fast(int center_x, int center_y, int size_param, uint16_t color) {
    int size = size_param + 4;
    int half_size = size / 2;
    int top_y = center_y - half_size;
    int bottom_y = center_y + half_size;
    int left_x = center_x - half_size;
    int right_x = center_x + half_size;
    lcd_draw_hline_fast(left_x, right_x - 1, top_y, color);
    lcd_draw_hline_fast(left_x, right_x - 1, bottom_y, color);
    lcd_draw_vline_fast(top_y + 1, bottom_y - 1, left_x, color);
    lcd_draw_vline_fast(top_y + 1, bottom_y - 1, right_x, color);
}

void lcd_draw_point_in_square(int x, int y, uint16_t color) {
    lcd_draw_2x2(x, y, color);
}

int point_x = 2200;
int point_y = 2200;
int point2_x = 2200;
int point2_y = 2200;

void update_point_position_with_inversion(int joystick_x, int joystick_y, int square_x, int square_y, int square_size, bool invert_x) {
    int max_x = 4095;
    int max_y = 4095;
    point_y = square_y + ((joystick_y * square_size) / max_y) - (square_size / 2);
    if (invert_x) {
        point_x = square_x + ((max_x - joystick_x) * square_size) / max_x - (square_size / 2);
    } else {
        point_x = square_x + ((joystick_x * square_size) / max_x) - (square_size / 2);
    }
    if (point_x < 0) point_x = 0;
    if (point_x >= LCD_WIDTH) point_x = LCD_WIDTH - 1;
    if (point_y < 0) point_y = 0;
    if (point_y >= LCD_HEIGHT) point_y = LCD_HEIGHT - 1;
}

void update_point2_position_with_inversion(int joystick_x, int joystick_y, int square_x, int square_y, int square_size, bool invert_x) {
    int max_x = 4095;
    int max_y = 4095;
    point2_y = square_y + ((joystick_y * square_size) / max_y) - (square_size / 2);
    if (invert_x) {
        point2_x = square_x + ((max_x - joystick_x) * square_size) / max_x - (square_size / 2);
    } else {
        point2_x = square_x + ((joystick_x * square_size) / max_x) - (square_size / 2);
    }
    if (point2_x < 0) point2_x = 0;
    if (point2_x >= LCD_WIDTH) point2_x = LCD_WIDTH - 1;
    if (point2_y < 0) point2_y = 0;
    if (point2_y >= LCD_HEIGHT) point2_y = LCD_HEIGHT - 1;
}

// GATT
static int gatt_svr_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    char joystick_data[32];
    snprintf(joystick_data, sizeof(joystick_data), "%d,%d,%d,%d,%d", last_x1, last_y1, last_x2, last_y2, last_sw1);
    
    os_mbuf_append(ctxt->om, joystick_data, strlen(joystick_data));
    ESP_LOGI(TAG, "GATT read: x1=%d, y1=%d, x2=%d, y2=%d, sw1=%d", last_x1, last_y1, last_x2, last_y2, last_sw1);
    return 0;
}

// GATT
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x180A),  // Device Information Service
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(0x2A29),  // Manufacturer Name
                .access_cb = gatt_svr_access_cb,  // Callback
                .flags = BLE_GATT_CHR_F_READ,
            },
            { 0 }
        },
    },
    { 0 }
};

// GAP-event
static int ble_gap_event_cb(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "✅ Connected: handle=%d", event->connect.conn_handle);
            } else {
                ESP_LOGE(TAG, "❌ Connection failed, status=%d", event->connect.status);
                ble_app_advertise();  // restart
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGW(TAG, "🔌 Disconnected, reason=%d", event->disconnect.reason);
            ble_app_advertise();
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "🔁 Advertising complete, restarting...");
            ble_app_advertise();
            return 0;

        default:
            ESP_LOGW(TAG, "Unhandled GAP event: %d", event->type);
            return 0;
    }
}

// Start BLE
static void ble_app_advertise(void) {
    struct ble_gap_adv_params adv_params = {0};
    struct ble_hs_adv_fields fields = {0};

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name = (uint8_t *)DEVICE_NAME;
    fields.name_len = strlen(DEVICE_NAME);
    fields.name_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    int rc = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_cb, NULL);
    if (rc == 0) {
        ESP_LOGI(TAG, "📣 Advertising started successfully");
    } else {
        ESP_LOGE(TAG, "⚠️ Failed to start advertising; error=%d", rc);
    }
}

// Callback afte BLE
static void ble_app_on_sync(void) {
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ESP_LOGI(TAG, "BLE host synchronized, address type: %d", ble_addr_type);
    ble_app_advertise();
}

// NimBLE host task
static void host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// main

void app_main() {
    lcd_init();
    lcd_clear_screen(0x0000); // Black background

    // Static text labels
    lcd_draw_text_fast("X:", 14, 15, 0x06db, 0x0000);
    lcd_draw_text_fast("Y:", 14, 25, 0x06db, 0x0000);
    lcd_draw_text_fast("SW:", 5, 35, 0x06db, 0x0000);
    lcd_draw_text_fast("X:", 100, 15, 0x07E0, 0x0000);
    lcd_draw_text_fast("Y:", 100, 25, 0x07E0, 0x0000);

    // Configure ADC and GPIO
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(X_1, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(Y_1, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(X_2, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(Y_2, ADC_ATTEN_DB_11);
    gpio_set_direction(SW_1, GPIO_MODE_INPUT);
    gpio_pullup_en(SW_1);

    // Draw squares for joystick indicators
    int square_size = 50;
    lcd_draw_square_fast(35, 75, square_size, 0xFFFF);
    lcd_draw_square_fast(120, 75, square_size, 0xFFFF);

    // Initialize variables
    char buf_x[16], buf_y[16], buf_sw[16], buf_x2[16], buf_y2[16];
    char old_buf_x[16], old_buf_y[16], old_buf_sw[16], old_x2[16], old_y2[16];
    
    // Initial readings
    int x = adc1_get_raw(X_1);
    int y = adc1_get_raw(Y_1);
    int sw = gpio_get_level(SW_1);
    int x2 = adc1_get_raw(X_2);
    int y2 = adc1_get_raw(Y_2);

    // Format initial values
    snprintf(buf_x, sizeof(buf_x), "%d", x);
    snprintf(buf_y, sizeof(buf_y), "%d", y);
    char ch_sw = (sw == 0) ? '+' : '-';
    snprintf(buf_sw, sizeof(buf_sw), "%c", ch_sw);
    snprintf(buf_x2, sizeof(buf_x2), "%d", x2);
    snprintf(buf_y2, sizeof(buf_y2), "%d", y2);

    // Draw initial values
    lcd_draw_text_fast(buf_x, 30, 15, 0xffff, 0x0000);
    lcd_draw_text_fast(buf_y, 30, 25, 0xffff, 0x0000);
    lcd_draw_text_fast(buf_sw, 30, 35, 0xffff, 0x0000);
    lcd_draw_text_fast(buf_x2, 120, 15, 0xffff, 0x0000);
    lcd_draw_text_fast(buf_y2, 120, 25, 0xffff, 0x0000);

    // Save initial values
    strcpy(old_buf_x, buf_x);
    strcpy(old_buf_y, buf_y);
    strcpy(old_buf_sw, buf_sw);
    strcpy(old_x2, buf_x2);
    strcpy(old_y2, buf_y2);

    // Initialize point positions
    update_point_position_with_inversion(x, y, 35, 75, square_size, true);
    update_point2_position_with_inversion(x2, y2, 120, 75, square_size, false);
    uint16_t point_color = (sw == 0) ? 0xf80c : 0x06db;
    lcd_draw_point_in_square(point_x, point_y, point_color);
    lcd_draw_point_in_square(point2_x, point2_y, 0x07E0);

    ESP_LOGI(TAG, "Starting app_main");

    nvs_flash_init();
    nimble_port_init();
    ESP_LOGI(TAG, "NimBLE port initialized");

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
    ESP_LOGI(TAG, "GATT services initialized");

    ble_hs_cfg.sync_cb = ble_app_on_sync;

    nimble_port_freertos_init(host_task);
    ESP_LOGI(TAG, "FreeRTOS NimBLE host task initialized");

    while (1) {
        // Read new values
        x = adc1_get_raw(X_1);
        y = adc1_get_raw(Y_1);
        sw = gpio_get_level(SW_1);
        x2 = adc1_get_raw(X_2);
        y2 = adc1_get_raw(Y_2);

        last_x1 = x;
        last_y1 = y;
        last_x2 = x2;
        last_y2 = y2;
        last_sw1 = sw;

        // Format new values
        snprintf(buf_x, sizeof(buf_x), "%d", x);
        snprintf(buf_y, sizeof(buf_y), "%d", y);
        ch_sw = (sw == 0) ? '+' : '-';
        snprintf(buf_sw, sizeof(buf_sw), "%c", ch_sw);
        snprintf(buf_x2, sizeof(buf_x2), "%d", x2);
        snprintf(buf_y2, sizeof(buf_y2), "%d", y2);

        // Update display only if values changed
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

        // Update and draw points
        lcd_draw_point_in_square(point_x, point_y, 0x0000); // Clear old position
        update_point_position_with_inversion(x, y, 35, 75, square_size, true);
        point_color = (sw == 0) ? 0xf80c : 0x06db;
        lcd_draw_point_in_square(point_x, point_y, point_color);

        lcd_draw_point_in_square(point2_x, point2_y, 0x0000); // Clear old position
        update_point2_position_with_inversion(x2, y2, 120, 75, square_size, true);
        lcd_draw_point_in_square(point2_x, point2_y, 0x07E0);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
