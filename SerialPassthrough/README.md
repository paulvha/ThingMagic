 **************************************************************************************
  /// Adjusted the default  SerialPassthrough sketch for connection with          ////
  /// Nano RFID simultanuous reader on softserial.                                ////
 *************************************************************************************
 
  It has been tested against the standard demo.c example from the Mercury API, accept 
  for the addition mentioned in point 2.
  
  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  This is not a stable connection for a number of reasons :
  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  1. SoftSerial and Nano must be at the same speed. The Nano will always be 
  at 115K after reboot. That is too fast for Softserial. During init we try connect
  with the lower speed. If that fails, we assume a reboot has happened: set
  SoftSerial to 115K and send a lower baudrate commmand and then reset SoftSerial to that 
  lower baudrate. However if the Nano was not either the requested baudrate of 115K nothing
  it will not connect. This can happen if the remote program (e.g. URA) has sent a command 
  to the reader to change baudrate. You need to remove the power from the reader and re-apply.

  2. As the USB port on an Arduino is opened the board will reset, driven by DTR.  More info 
  on https://forum.arduino.cc/index.php?topic=566529.0
  
  That means that the sketch will try to connect again to the Nano, and then loosing the first
  request sent by the remote (e.g. URA). When URA connects it will try to loop through 
  different baudrates to find one where the reader is working on. The changes are very
  high that the remote program (e.g. URA) has switched baudrate before the Nano connection ended.
  Now the Arduino and remote program are out of sync.
  if you build your own program with the Mercury API, To prevent looping. you can try to use 
  the following code BEFORE calling connect():
  
  // set baudrate
  uint32_t baudrate = 19200;                            // MUST be the same as serial speed in sketch
  ret = TMR_paramSet(rp,TMR_PARAM_BAUDRATE,&baudrate);
  
  if(ret != TMR_SUCCESS)
  {
    printf("could not set baudrate\n");
    TMR_destroy(rp);
    exit(0);
  }
  
  3. softSerial on has a default RX buffer of 64 characters. The chances of loosing characters
  due to buffer overrun is high. Keep reading Serial speed faster or equal to the Nano to prevent
  buffer overrun as much as possible

  On failure to connect the on-board led will blink every second.
  
  NOTE 1:
  You can NOT just connect on pin 0 and 1 (the standard serial). Even with an empty sketch the driver 
  on the Arduino will trigger interrupt with each character that arrives from the Nano. 
  
  NOTE 2:
  Always make sure to power the RFID shield with a good power supply. Many communication failures come from that as well

Paul van Haastrecht November 2018
