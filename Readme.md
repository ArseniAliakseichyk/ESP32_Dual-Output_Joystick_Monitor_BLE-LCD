# ESP32 Joystick BLE Monitor

This project uses an ESP32 board and two joysticks. It shows joystick positions on a small LCD screen and also sends data using Bluetooth (BLE). A Python script can read this data and show it in the terminal with simple graphics.

🆕 The code is now modular. It is split into folders for LCD, BLE, input, and main logic.

## ✅ What it does
- Reads joystick X/Y positions and button
- Shows numbers and moving dots on LCD screen
- Sends joystick data with Bluetooth (BLE)
- Displays real-time data in terminal using Python
- Modular structure in both C and Python


---

## 📦 What you need

- ESP32 board  
- 1.8" TFT LCD (160x128)  
- 2 analog joysticks  
- Wires and breadboard  
- Computer with Python 3.9+ and BLE support  

---

## 🖥️ Python BLE Console Monitor

You can view the joystick data in your terminal using the Python script.

### ▶️ How to run it

1. Go to the `monitor/` folder
2. Install Python packages:
```bash
pip install -r .\python-code\requirements.txt
```
3. Run the monitor:
```bash
python .\python-code\visualization-j.py
```

It will find the BLE device and show joystick data as numbers and moving dots using `curses`.

---

## 📡 BLE Details

- Device name: **BLE-Arseni**
- BLE Service UUID: `0x180A` (Device Info)
- Characteristic UUID: `0x2A29` (Manufacturer Name)
- Sent data format:

---

## 🚀 How to Start

1. Connect wires to ESP32 (see table above)
2. Build and flash code using ESP-IDF:
```bash
idf.py build
idf.py flash
```
3. Open serial monitor or BLE app
4. Optionally, use the Python script to monitor in terminal

---

<!-- ## 📹 Demo Video

[Demo Video Link Here](#)

--- -->

## 🧠 What I learned

- How to use BLE on ESP32  
- How to show graphics on LCD  
- How to split embedded code into modules  
- How to build a real-time terminal GUI in Python  
- How to send and parse BLE data  

---

## 💬 Easy Explanation

"This project reads two game sticks with ESP32. It shows values and moving dots on a small screen. It also sends data to the computer with Bluetooth. A Python program shows this data live in the terminal."