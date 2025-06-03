#pragma once

typedef struct {
    int x1;
    int y1;
    int x2;
    int y2;
    int sw1;
} JoystickData;

void input_init(void);
JoystickData input_read(void);