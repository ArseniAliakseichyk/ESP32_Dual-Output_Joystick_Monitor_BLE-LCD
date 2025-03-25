# ESP32 Joystick BLE Monitor

This project uses an ESP32 board and two joysticks. It shows joystick positions on a small LCD screen. It also sends the data using Bluetooth (BLE).

## ✅ What it does
- Reads joystick X/Y positions and button
- Shows numbers and moving dots on screen
- Sends the data with Bluetooth (BLE)
- Updates fast (50 times per second)

## 📦 What you need
- ESP32 board
- 1.8" TFT LCD (160x128)
- 2 analog joysticks
- Some wires

## 🔌 Pin Connections

| ESP32 Pin | Joystick 1 | Joystick 2 | LCD      |
|-----------|------------|------------|----------|
| GPIO0     | X          |            |          |
| GPIO1     | Y          |            |          |
| GPIO2     |            | Y          |          |
| GPIO3     |            | X          |          |
| GPIO9     | Button     |            |          |
| GPIO4     |            |            | MOSI     |
| GPIO5     |            |            | CLK      |
| GPIO6     |            |            | CS       |
| GPIO7     |            |            | DC       |
| GPIO8     |            |            | RST      |

## 🔁 How it works
1. ESP32 reads joystick data
2. LCD shows:
   - X and Y values
   - + or – if button is pressed
   - Dots that move when joysticks move
3. ESP32 sends values with BLE
4. Updates 50 times per second

## 📡 Bluetooth (BLE)
- Name: **BLE-Arseni**
- Service: Device Info (0x180A)
- Characteristic: Manufacturer Name (0x2A29)
- Data: `x1,y1,x2,y2,button`

## 🚀 How to start
1. Connect wires like the table
2. Upload code to ESP32
3. Power the board
4. Use BLE app to see joystick data

<!-- ## 📹 Demo Video
[See the video demo here](#)

![Diagram](project_diagram.png) -->

## 🧠 What I learned
- How to use BLE with ESP32
- How to show graphics on LCD
- How to read analog sensors
- How to update screen fast
- How to mix software and hardware

## ✨ Why it's a good project
- Shows hardware and coding skills
- Uses LCD and Bluetooth together
- Full project with clean code
- Good for internships

## 💬 Easy explanation
"This project uses ESP32 to read two joysticks. It shows values and moving dots on screen. It sends the data to phone using Bluetooth. When you move the stick, the dot moves too. Button shows + when pressed."
