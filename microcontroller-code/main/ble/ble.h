#pragma once
#include <stdint.h>

void ble_init();
void ble_update_joystick_data(int x1, int y1, int x2, int y2, int sw);