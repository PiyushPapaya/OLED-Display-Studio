# OLED Display Studio

A wireless OLED display controller that lets you upload images and animations to an Arduino Nano 33 BLE from your browser — no cables needed after flashing.

**Live App:** [oled-display-studio.vercel.app](https://oled-display-studio.vercel.app/)*

---

## How It Works

1. **Flash** the Arduino sketch to your Nano 33 BLE (one-time USB step)
2. **Open** the web app in Chrome (desktop or Android)
3. **Connect** to the Arduino via Web Bluetooth
4. **Upload** images or GIFs — they get converted to 128×64 monochrome bitmaps and sent over BLE
5. **Watch** your OLED display play the animation wirelessly

The web app handles all image processing: resizing, dithering, GIF frame extraction, and BLE chunked transfer. The Arduino stores up to 60 frames in RAM and plays them back in a loop.

---

## Hardware Required

| Component | Details |
|---|---|
| **Arduino Nano 33 BLE** | Must be the BLE variant (has nRF52840 chip) |
| **SH1106 128×64 OLED** | I2C version (4 pins). SSD1306 also works — see note below |
| **Jumper wires** | 4× female-to-female (or as needed for your setup) |

### Wiring

```
Arduino Nano 33 BLE          SH1106 OLED Display
─────────────────────        ───────────────────
3.3V  ──────────────────────  VCC
GND   ──────────────────────  GND
A4 (SDA)  ──────────────────  SDA
A5 (SCL)  ──────────────────  SCL
```

> **Using SSD1306 instead of SH1106?** Open `oled_display_studio.ino` and swap the display constructor:
> ```cpp
> // Change this:
> U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
> // To this:
> U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
> ```

---

## Setup

### 1. Flash the Arduino

1. Open `oled_display_studio/oled_display_studio.ino` in the Arduino IDE
2. Install these libraries via **Library Manager**:
   - `ArduinoBLE`
   - `U8g2`
3. Select board: **Arduino Nano 33 BLE**
4. Upload the sketch

The OLED should show "Ready!" and "Waiting for connection..." once flashed.

### 2. Use the Web App

Open the app in **Google Chrome** (Web Bluetooth is required — not supported in Firefox/Safari):

- **Hosted:** Visit your deployed Vercel URL
- **Local:** Run `npx serve .` in this folder, then open `http://localhost:3000`

Then:

1. Click **Connect Device** and select "Nano33_OLED_Pro" from the Bluetooth popup
2. Drag & drop an image or GIF into the upload area
3. Click **Send to Display**
4. Adjust animation speed with the slider if needed

### Supported Formats

- **PNG / JPG** — uploaded as a single static frame
- **Animated GIF** — automatically split into individual frames (up to 60)
- Images are resized to 128×64 and dithered to monochrome automatically

---

## Features

- Wireless BLE image transfer (no serial cable needed)
- Animated GIF support with frame extraction
- Live 128×64 preview before sending
- Adjustable animation speed (50ms – 60s per frame)
- Auto-reconnect on connection drop
- Frame-by-frame navigation in the preview
- Drag & drop or file picker upload
- Works on desktop Chrome and Android Chrome

---

## Deployment (Vercel)

The web app is a single static HTML file — no build step needed.

```bash
npm i -g vercel
vercel --prod
```

Or connect this repo to [vercel.com](https://vercel.com) for automatic deploys on push.

---

## Project Structure

```
├── index.html                              # Web app (single-file, no dependencies)
├── oled_display_studio/
│   └── oled_display_studio.ino             # Arduino firmware
├── vercel.json                             # Vercel deployment config
├── package.json                            # Project metadata
└── README.md
```

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Bluetooth popup doesn't show the device | Make sure the Arduino is powered on and not connected to another device |
| "BLE Error!" on OLED screen | Check wiring — SDA/SCL might be swapped |
| Display stays black | Try switching to the SSD1306 constructor (see wiring section) |
| Frames don't send | Refresh the browser, reconnect, and try again |
| Chrome says "Bluetooth not supported" | Use Chrome on desktop or Android — iOS and Firefox don't support Web Bluetooth |

---

## License

MIT
=======
# OLED-Display-Studio
A wireless OLED display controller that lets you upload images and animations to an Arduino Nano 33 BLE from your browser.
>>>>>>> 0df2d1bbd5962a18eb8cc9110d10a848dfc66ff4
