import time
import socket
import subprocess
from PIL import Image, ImageDraw, ImageFont
import Adafruit_SSD1306

disp = Adafruit_SSD1306.SSD1306_128_32(rst=None, i2c_address=0x3C)
disp.begin()
disp.clear()
disp.display()
width = disp.width
height = disp.height
font = ImageFont.load_default()

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        return s.getsockname()[0]
    except:
        return None
    finally:
        s.close()

def get_ssid():
    try:
        ssid = subprocess.check_output(
            "iwgetid wlan0 -r", shell=True, text=True
        ).strip()
        return ssid if ssid else None
    except:
        return None

while True:
    ssid = get_ssid()
    ip = get_ip() if ssid else None

    image = Image.new("1", (width, height))
    draw = ImageDraw.Draw(image)

    if ssid and ip:
        draw.text((0, 0), "Connected to:", font=font, fill=255)
        draw.text((0, 10), ssid, font=font, fill=255)
        draw.text((0, 20), f"IP: {ip}", font=font, fill=255)
    else:
        draw.text((0, 10), "Checking...", font=font, fill=255)

    disp.image(image)
    disp.display()
    time.sleep(20)

