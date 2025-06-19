#include "ble.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define TAG "BLE_SIMPLE"
#define DEVICE_NAME "BLE-Arseni"
#define SERVICE_UUID BLE_UUID16_DECLARE(0x1234)
#define CHARACTERISTIC_UUID BLE_UUID16_DECLARE(0x5678)

static uint8_t ble_addr_type;
static int last_x1, last_y1, last_x2, last_y2, last_sw1;

static void ble_app_advertise(void);

static int ble_gap_event_cb(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "BLE GAP event: Connected");
            return 0;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "BLE GAP event: Disconnected");
            ble_app_advertise();
            return 0;
        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "BLE GAP event: Advertising complete");
            ble_app_advertise();
            return 0;
    }
    return 0;
}

static void ble_app_on_sync(void) {
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ESP_LOGI(TAG, "BLE host synchronized, address type: %d", ble_addr_type);
    ble_app_advertise();
}

static int gatt_svr_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    char joystick_data[32];
    snprintf(joystick_data, sizeof(joystick_data), "%d,%d,%d,%d,%d", last_x1, last_y1, last_x2, last_y2, last_sw1);
    
    os_mbuf_append(ctxt->om, joystick_data, strlen(joystick_data));
    ESP_LOGI(TAG, "GATT read: x1=%d, y1=%d, x2=%d, y2=%d, sw1=%d", last_x1, last_y1, last_x2, last_y2, last_sw1);
    return 0;
}

static void host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_update_joystick_data(int x1, int y1, int x2, int y2, int sw) {
    last_x1 = x1;
    last_y1 = y1;
    last_x2 = x2;
    last_y2 = y2;
    last_sw1 = sw;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = SERVICE_UUID,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = CHARACTERISTIC_UUID,
                .access_cb = gatt_svr_access_cb,
                .flags = BLE_GATT_CHR_F_READ,
            },
            { 0 }
        },
    },
    { 0 }
};

static void ble_app_advertise(void) {
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));
    
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)DEVICE_NAME;
    fields.name_len = strlen(DEVICE_NAME);
    fields.name_is_complete = 1;
    
    ble_gap_adv_set_fields(&fields);
    
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_cb, NULL);
}

void ble_init() {
    ESP_LOGI(TAG, "Initializing BLE");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nimble_port_init();
    
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
    
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    
    nimble_port_freertos_init(host_task);
}