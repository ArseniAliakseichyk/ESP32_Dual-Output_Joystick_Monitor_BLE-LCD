idf_component_register(SRCS "main.c"
    SRCS 
        "main.c"
        "ble/ble.c"
        "input/input.c"
        "lcd/lcd.c"
    INCLUDE_DIRS 
        "."
        "./ble"
        "./input"
        "./lcd"
)
