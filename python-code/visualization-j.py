# | pip install -r requirements.txt | 

import asyncio
from bleak import BleakClient, BleakScanner
import curses
import time

JOYSTICK_BOX_HEIGHT = 19
JOYSTICK_BOX_WIDTH = JOYSTICK_BOX_HEIGHT * 2
COLOR_MAIN = curses.COLOR_CYAN
COLOR_SW_ON = curses.COLOR_GREEN
COLOR_SW_OFF = curses.COLOR_RED

class JoystickDisplay:
    def __init__(self, stdscr):
        self.stdscr = stdscr
        self.init_colors()

        height, width = self.stdscr.getmaxyx()
        self.left_win = curses.newwin(height - 2, width // 2 - 2, 1, 1)
        self.right_win = curses.newwin(height - 2, width // 2 - 2, 1, width // 2 + 1)

        self.prev_pos = {'left': None, 'right': None}
        self.last_sw = False

        self.draw_static_elements()

    def init_colors(self):
        curses.start_color()
        curses.init_pair(1, COLOR_MAIN, curses.COLOR_BLACK)
        curses.init_pair(2, COLOR_SW_ON, curses.COLOR_BLACK)
        curses.init_pair(3, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
        curses.init_pair(4, COLOR_SW_OFF, curses.COLOR_BLACK)

    def draw_static_elements(self):
        self.stdscr.clear()
        self.stdscr.border()
        self.left_win.border()
        self.right_win.border()

        self.left_win.addstr(0, 2, " Joystick 1 ", curses.color_pair(1))
        self.right_win.addstr(0, 2, " Joystick 2 ", curses.color_pair(2))

        self.left_win.addstr(2, 2, "X :", curses.color_pair(1))
        self.left_win.addstr(3, 2, "Y :", curses.color_pair(1))
        self.left_win.addstr(4, 2, "SW:", curses.color_pair(1))

        self.right_win.addstr(2, 2, "X :", curses.color_pair(2))
        self.right_win.addstr(3, 2, "Y :", curses.color_pair(2))

        self.draw_box(self.left_win, 7)
        self.draw_box(self.right_win, 7)

        self.stdscr.refresh()

    def draw_box(self, win, y_pos):
        top = "┌" + "─" * JOYSTICK_BOX_WIDTH + "┐"
        middle = "│" + " " * JOYSTICK_BOX_WIDTH + "│"
        bottom = "└" + "─" * JOYSTICK_BOX_WIDTH + "┘"

        win.addstr(y_pos - 1, 1, top)
        for i in range(JOYSTICK_BOX_HEIGHT):
            win.addstr(y_pos + i, 1, middle)
        win.addstr(y_pos + JOYSTICK_BOX_HEIGHT, 1, bottom)

    def update_data(self, data):
        self.last_sw = data['sw1']

        self.left_win.addstr(2, 6, f"{data['x1']:4d}")
        self.left_win.addstr(3, 6, f"{data['y1']:4d}")
        self.left_win.addstr(4, 6, " + " if data['sw1'] else " - ")

        self.right_win.addstr(2, 6, f"{data['x2']:4d}")
        self.right_win.addstr(3, 6, f"{data['y2']:4d}")

        self.update_dot(self.left_win, 7, data['x1'], data['y1'], 'left')
        self.update_dot(self.right_win, 7, data['x2'], data['y2'], 'right')

        self.left_win.refresh()
        self.right_win.refresh()

    def update_dot(self, win, y_pos, x_val, y_val, side):
        x_range = JOYSTICK_BOX_WIDTH - 2
        y_range = JOYSTICK_BOX_HEIGHT

        x = 1 + (x_range - 2 - int((x_val / 4095) * (x_range - 2)))
        y = min(y_range - 1, int((y_val / 4095) * (y_range - 1)))

        prev = self.prev_pos[side]
        if prev and prev != (x, y):
            win.addch(y_pos + prev[1], 2 + prev[0], ' ')

        if side == 'left':
            color = curses.color_pair(1) if self.last_sw else curses.color_pair(3)
        else:
            color = curses.color_pair(2)

        win.addch(y_pos + y, 2 + x, '◉', color)
        self.prev_pos[side] = (x, y)

class BLEJoystickReader:
    def __init__(self, stdscr):
        self.stdscr = stdscr
        self.display = None
        self.client = None
        self.device_name = "BLE-Arseni"

    async def connect_with_animation(self):
        max_wait = 30
        start_time = time.time()
        dot_index = 0
        dots = ["   ", ".  ", ".. ", "..."]

        while time.time() - start_time < max_wait:
            self.stdscr.clear()
            self.stdscr.border()
            self.stdscr.addstr(5, 10, f"Searching for device {self.device_name}{dots[dot_index % 4]}")
            self.stdscr.refresh()
            dot_index += 1

            device = await BleakScanner.find_device_by_name(self.device_name, timeout=1.0)
            if device:
                self.client = BleakClient(device.address)
                await self.client.connect()
                return True
            await asyncio.sleep(0.3)

        self.stdscr.clear()
        self.stdscr.addstr(7, 10, f"Device {self.device_name} not found.")
        self.stdscr.addstr(8, 10, "Press any key to exit.")
        self.stdscr.refresh()
        self.stdscr.getch()
        return False

    async def read_data(self):
        raw = await self.client.read_gatt_char("00002a29-0000-1000-8000-00805f9b34fb")
        data = raw.decode().strip().split(',')
        return {
            'x1': int(data[0]), 'y1': int(data[1]),
            'x2': int(data[2]), 'y2': int(data[3]),
            'sw1': bool(int(data[4]))
        }

    async def run(self):
        if await self.connect_with_animation():
            self.display = JoystickDisplay(self.stdscr)
            try:
                while True:
                    data = await self.read_data()
                    self.display.update_data(data)
                    await asyncio.sleep(0.015)
            finally:
                await self.client.disconnect()

def main(stdscr):
    curses.curs_set(0)
    stdscr.timeout(0)
    reader = BLEJoystickReader(stdscr)
    asyncio.run(reader.run())

if __name__ == "__main__":
    curses.wrapper(main)
