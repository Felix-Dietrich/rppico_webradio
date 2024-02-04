# rppico_webradio<br>Felix Dietrich 2024
# Internet-Based Radio

## FAQ<br>

- **How can the SSID and password be entered when there is no internet connection yet?**<br>

- **What should I consider when building the project on a breadboard?**<br>
  Establish a clear concept for the 5V power supply and the power rails.<br>
  Don't forget the Schottky diode on +5V!<br>
  [Use the breadboard photo as a guide.](./images/current/BreadBoard1.jpg)<br>
  
  Black = GND<br>
  Red = +5V<br>
  Blue = 3.3V supplied by the Pi Pico<br>
  Yellow = Analog inputs<br>
  Orange = I2S data lines<br>
  Green = Speaker output<br>
  
  
- **What needs to be considered before the project can be compiled?**<br>
  Make sure you have installed the Pi Pico SDK, and you can compile the sample projects.<br>
  [See Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)<br>
  Then, the CMakeLists.txt file in the root folder rppico_webradio needs to be modified.
  
  Everywhere you see a :-), you need to adjust the paths to match your installation.<br>
  If you are using Windows, you need to add an elseif(WIN32) statement.<br>
  
  

EOF :-)





