# version 1.0  november 2018

This sketch will connect the ESP8266 thing (https://www.sparkfun.com/products/13231 )to the local network as a server and constant tries to read RFID tags with a Sparkfun/nano/simultaneousreader ( https://www.sparkfun.com/products/14066 ) as well as check for client connection. 

The detected unique EPC's are stored in an array on the board and can be accessed with the following commands:
- get_cnt        : return the number of unique EPC's in the array
- get_epc        : returns the unique EPC numbers
- clr_epc        : will remove the EPC's from the array after the next get_epc command
- FORCED_clr_epc : will clear all the EPC's from the array immediately 
    
Optional if you had also connected a DS18x20 temperature sensor :
- temp_c : will return temperature in celsius
- temp_f : will return temperature in Fahrenheit

Next to the .ino there is also a document included for detailed description

Hardware connections
  
- RFID reader --------- ESP8266 thing
- GND ------------------GND
- UART RX ------------- 13
- UART TX ------------- 12
  
The reader was external powered in my case. if you want to use the 5V from the ESP8266
connect a wire between UART VCC ------ 5V ESP8266
 
Set the switch to HW UART
 
 
Optional Temperature sensor ( DS18x20 family)

- DS18x20 ------------ ESP8266 thing
- GND   ------------- GND
- 5V /red -----------  5V
- data/yellow -------- 4
 
You MUST connect a resistor of 4K7 between data and +5v
