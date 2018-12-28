# version 1.0  December 2018 / 

Paul van Haastrecht paulvha@hotmail.com

The sketch will connect the ESP32 thing (https://www.sparkfun.com/products/13907) to the local network as a server and constant tries to read RFID tags with a Sparkfun/nano/simultaneousreader ( https://www.sparkfun.com/products/14066) as well as check for client connection.
The detected unique EPC's are stored in an array on the board and can be accessed with the following commands:

epc_cnt          : return the number of unique EPC's detected
epc_num          : returns the unique EPC numbers comma seperated(EPC NUMBER,)
epc_dct          : same as epc_num, but will add to each EPC how often detected (NUMBER:##,)
epc_all          : perform get_cnt + epc_dct commands
epc_clr          : will remove the EPC's from the array after the next epc_num command
force_epc_clr    : will clear all the EPC's from the array immediately

Optional if you had also connected a DS18x20 temperature sensor :
tmp_c         : returns temperature in Celsius
tmp_f         : returns temperature in Fahrenheit

Next to the .ino there is also a document included for detailed description

Hardware connections

   RFID reader          ESP32 thing (Serial2 is used)
     GND ---------------   GND
     UART TX ---- LS-----  16      (LS = level shifter !!)
     UART RX ------------  17

The reader is external powered in my case. 

WARNING!! WARNING !!WARNING!! WARNING !!WARNING!!

The NANO M6E needs at least 3.7V VCC. It will not work stable on 3.3V.
 
If you connect to the 5V (USB-V) you MUST use a level shifter for the for RX line from RFID reader to the ESP32.  The pins on the ESP-32 can only handle 3V3 signals !!
 
The RFID RX line, TX / pin 17 from ESP32, actually does not need a level shifter. The 3V3 that is coming from the ESP32 works good for the Nano. HOWEVER for the TX FROM the Nano, did not work well with that level shifter (https://www.sparkfun.com/products/12009)
Apparently the level shifter on the Nano and this level shifter do not work well together. So I made it working with resistors: 
 
             ______              _____
  GND ------| 10K  |-----!---- | 5k6 |------  TX from Nano
            --------     !     -------  
                         !
                     pin 16 (ESP32)

Optional temperature sensor (DS18x20 family) 

              DS18x20           ESP32 thing
     ____      GND   ------------- GND
 .--| 4k7 |--  VCC /red ---------  3V3
 |   ‘-----’ 
 ‘-----------  data/yellow -------  4 

You MUST connect a resistor of 4K7 between data and VCC for pull-up.
 
WARNING!! WARNING !!WARNING!! WARNING !!WARNING!! 

The DS18B20 works with VCC between 3 and 5V. It works fine on 3.3V however if connected to 5V you MUST include a level shifter or making a bridge with resistors like below
            ______              _____
  GND ------| 10K  |-----!---- | 5k6 |------ data/yellow from DS18x20
            --------     !     -------  
                         !
                      pin 4 (ESP32)
