# IoT-Button
This is an ESP8266 Arduino Configurable Internet of Things button via Wi-Fi to trigger an event at with ifttt.com

What you need
  - An Esp8266-12E or compatible module with a reset button
  - Wifi capable device
  - A wifi network SSID and password
  - ifttt event name
  - ifttt key

# Operation and feedback
  - A reset Button
  - A blinking Led
  - Serial port (115200)
  

# Configuration
  - You need an account at http://ifttt.com.
  - Get your key at https://ifttt.com/maker.
  - To get an event name create a receipe that includes a Maker trigger.
  - After flashing the device power cycle the module.
  - Access via wi-fi to the hotspot "BUTTON" and set your web browser to 192.168.1.1
  - On first use the led blinks continuosly (setup Mode) fill the configurations fields and power cicle the module.
  
# How it works?
  Once you set your configuration everytime you reset the module tries to connect to wifi network, once connected it triggers the ifttt event and enters info mode at SSID: BUTTON and IP 192.168.1.1 (same as configuration). If you not access info mode in one minute it sleeps. If you access you got 10 minutes and then it sleeps.

  You can clear your settings on info mode.
  
  ![Flow chart](https://raw.githubusercontent.com/peekpt/IoT-Button/master/chart.png)
  

# LED feedback
  - Configuration Mode -> continuous blinking.
  - Connected to Wifi -> 1 Blink.
  - Successfully triggered event -> 5 blinks.
  - Failed -> 2 blinks.

# Flash the sketch
  To enter FLASH MODE Pull up CH_PD with a resitor (1k) to 3.3v permanently. To flash Power cycle the device with GPIO0 set to GND.
  To enter running mode remove GPIO0 from GND and leave the pullup 3.3v resistor.
  You need the Arduino IDE. 
  Add this url "http://arduino.esp8266.com/stable/package_esp8266com_index.json" on settings in the Aditional Board Manager URLs field.
  Install the Esp8266 community boards on Boards Manager (Tools>Boards>Boards Manager) 
  Select your module.
  
Don't forget to power cycle the module everytime you flash the firmware.

# Credits
  - Based on Garth V. Houwen IoT Button https://github.com/garthvh/esp8266button 
  
# License
MIT License
