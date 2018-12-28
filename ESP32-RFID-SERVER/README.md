# version 1.0  December 2018 / 

Paul van Haastrecht paulvha@hotmail.com

The sketch will connect the ESP32 thing (https://www.sparkfun.com/products/13907) to the local network as a server and constant tries to read RFID tags with a Sparkfun/nano/simultaneousreader ( https://www.sparkfun.com/products/14066) as well as check for client connection.
The detected unique EPC's are stored in an array on the board and can be accessed with the following commands:

.epc_cnt          : return the number of unique EPC's detected

.epc_num          : returns the unique EPC numbers comma seperated(EPC NUMBER,)

.epc_dct          : same as epc_num, but will add to each EPC how often detected (NUMBER:##,)

.epc_all          : perform get_cnt + epc_dct commands

.epc_clr          : will remove the EPC's from the array after the next epc_num command

.force_epc_clr    : will clear all the EPC's from the array immediately

Optional if you had also connected a DS18x20 temperature sensor :

.tmp_c         : returns temperature in Celsius

.tmp_f         : returns temperature in Fahrenheit

Next to the .ino there is also a document included for detailed description

Hardware connections: 
See in top of sketch or the detailed document

