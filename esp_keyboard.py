import serial
import pyautogui
import time

ser = serial.Serial('COM11', 115200, timeout=1)

time.sleep(2)

print("ESP32 Keyboard Controller Started")

while True:

    if ser.in_waiting:
        data = ser.readline().decode(errors='ignore').strip()

        print("Received:", data)

        if data == "RIGHT":
            pyautogui.press('right')
            print("RIGHT ARROW")

        elif data == "LEFT":
            pyautogui.press('left')
            print("LEFT ARROW")