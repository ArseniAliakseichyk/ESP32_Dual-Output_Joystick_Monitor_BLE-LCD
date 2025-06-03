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

static uint8_t ble_addr_type;
static int last_x1, last_y1, last_x2, last_y2, last_sw1;

static void ble_app_advertise(void);
// GAP-event
static int ble_gap_event_cb(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Connected: handle=%d", event->connect.conn_handle);
            } else {
                ESP_LOGE(TAG, "Connection failed, status=%d", event->connect.status);
                ble_app_advertise();  // restart
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGW(TAG, "🔌 Disconnected, reason=%d", event->disconnect.reason);
            ble_app_advertise();
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "Advertising complete, restarting...");
            ble_app_advertise();
            return 0;

        default:
            ESP_LOGW(TAG, "Unhandled GAP event: %d", event->type);
            return 0;
    }
}
// Callback afte BLE
static void ble_app_on_sync(void) {
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ESP_LOGI(TAG, "BLE host synchronized, address type: %d", ble_addr_type);
    ble_app_advertise();
}
// GATT
static int gatt_svr_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    char joystick_data[32];
    snprintf(joystick_data, sizeof(joystick_data), "%d,%d,%d,%d,%d", last_x1, last_y1, last_x2, last_y2, last_sw1);
    
    os_mbuf_append(ctxt->om, joystick_data, strlen(joystick_data));
    ESP_LOGI(TAG, "GATT read: x1=%d, y1=%d, x2=%d, y2=%d, sw1=%d", last_x1, last_y1, last_x2, last_y2, last_sw1);
    return 0;
}
// NimBLE host task
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
        ESP_LOGI(TAG, "Advertising started successfully");
    } else {
        ESP_LOGE(TAG, "Failed to start advertising; error=%d", rc);
    }
}
void ble_init() {
    ESP_LOGI(TAG, "Initializing BLE");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nimble_port_init();
    
    // Initialize GAP and GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();
    
    // Configure GATT services
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
    
    // Set synchronization callback
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    
    // Start host task
    nimble_port_freertos_init(host_task);
}