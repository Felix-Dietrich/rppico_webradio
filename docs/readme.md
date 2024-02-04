# rppico_webradio<br>Felix Dietrich 2024
# Project Description: Internet-Based Radio

### 
Last update February 3, 2024


## Features
- Internet connection via WLAN
- Operation using analog rotary knobs, resembling those of an analog radio
- Storage of 11 MP3 streams through a Web Interface
- Output power: 3W Class D mono
- 10-channel graphic equalizer with real-time feedback
- Battery status
- Clear segmentation of signal processing within individual RTOS tasks
- Charging and program update via USB
- USB command terminal for internal monitoring
- Written in C-language

## Components
- RPi Pico W
- Max 98357A
- Potentiometer 100k for volume control
- Potentiometer 100k for station selection
- Speaker or speaker box (4-8 ohms)
- Power supply: 5V 800mA


## Sources
- [Radio SW github](https://github.com/Felix-Dietrich/rppico_webradio)

## Schematics and layouts
 - [Schematics and PCB (KiCad)](https://github.com/Felix-Dietrich/rppico_webradio_kicad)
 - [Schematic quick view](./images/current/schematic1.png)

## Datasheets
- [RPi Pico Datasheet](https://datasheets.raspberrypi.com/pico/pico-datasheet.pdf)

- [Class D Amp MAX98357A](https://www.analog.com/media/en/technical-documentation/data-sheets/max98357a-max98357b.pdf)

- [Class D Amplifier Breakout - MAX98357A](https://www.adafruit.com/product/3006)

## How it works
TBD
- Process diagrams
- Fetching the mp3 stream
- The MP3 decoder
  - Why MP3?
- The equalizer
  - Filtertypes
  - Time vs. frequency domain
- The I2S Interface
- Some considerations on timing
- The Audio amplifier
- Volume Control and station selection


## To-Dos
- [todo.md](todo.md)


## Images
- [Breadboard](./images/current/BreadBoard1.jpg)
- [Breadboard small speaker box](./images/current/BreadBoardSmallSpeakerBox1.jpg)
- [Drive big speaker box;-)](./images/current/DriveBigSpeaker1.jpg)
- [PCB](./images/current/PCB1.jpg)
- [Board installed and wired](./images/current/PCBWired1.jpg)
- [MP3-Radio front](./images/current/MP3RadioFront1.jpg)
- [MP3-Radio top](./images/current/MP3RadioTop1.jpg)



## The story behind

TBD<br>
In the year 2005 2005...<br>



## Images
- [FM-Radio front](./images/history/FMRadioFront.JPG)
- [FM-Radio rear](./images/history/FMRadioRear.JPG)
- [FM-Radio inside](./images/history/FMRadioInside.JPG)


## FAQ
- [faq.md](faq.md)


EOF :-)





