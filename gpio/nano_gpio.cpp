/*********************************************************************
 * Section added for GPIO control
 * version 1.0 paulvha August 2019
 *
 * Be aware the pins can only handle 3.3V (as the rest of the Nano)
 *********************************************************************/
/* Set GPIO output level
 *
 * @param gpio: GPIO to set between GPIO1 and LV4
 *
 * @param high
 *  HIGH set to high / 1 .
 *  LOW  set to low / 0 .
 *
 * Return code :
 * OK = ALL_GOOD
 * else error
 */
uint8_t RFID::setGPIO(uint8_t gpio, bool high)
{
  uint8_t data[2];
  if (gpio < GPI01 || gpio > LV4) return(ERROR_UNKNOWN_OPCODE);

  data[0] = gpio;
  data[1] = (high == true);
  sendMessage(TMR_SR_OPCODE_SET_USER_GPIO_OUTPUTS, data, sizeof(data));

  return(msg[0]);
}

/* read the GPIO status
 *
 * @param gpio: between GPIO1 and LV4
 *
 * @param *state
 *  HIGH is current reading is high.
 *  LOW  if current reading is low.
 *
 * Return code :
 * OK = ALL_GOOD
 * else error (ignore *state)
 *
 * answer looks like
 * 0  [FF]             header
 * 1  [0D]             length of data
 * 2  [66]             opcode
 * 3  [00] [00]        response code
 * 5  [01]             block count
 * 6  [01] [00] [01]   ID = 01 , output (1) / input (0), state high / low
 * 9  [02] [00] [00]    ""
 * 12 [03] [00] [00]    ""
 * 15 [04] [00] [01]    ""
 */
uint8_t RFID::getGPIO(uint8_t gpio, bool *state)
{
  uint8_t data = 0x01;

  if (gpio < GPI01 || gpio > LV4) return(ERROR_UNKNOWN_OPCODE);

  sendMessage(TMR_SR_OPCODE_GET_USER_GPIO_INPUTS, &data, 1);

  *state = (msg[(8 + (gpio-1) * 3)] == 1);

  return(msg[0]);
}

/* set GPIO direction
 *
 * @param gpio :  requested gpio / pin between GPIO1 and LV4
 *
 * @param out :
 *  GPIOout(1) for output
 *  GPIOin (0) for input
 *
 * @value :
 *  set in case of output (default = low/false)
 *
 * Return code :
 * OK = ALL_GOOD
 * else error
 */
uint8_t RFID::setGPIODirection(uint8_t gpio, bool out, bool value)
{
  uint8_t data[4];

  if (gpio < GPI01 || gpio > LV4) return(ERROR_UNKNOWN_OPCODE);

  data[0] = 1;  /* Option flag */
  data[1] = gpio;
  data[2] = (out == true) ? 1 : 0;
  data[3] = (value == true) ? 1 : 0;  /* New value if output */

  sendMessage(TMR_SR_OPCODE_SET_USER_GPIO_OUTPUTS, data, sizeof(data));

  return(msg[0]);
}

/* get the GPIO direction
 *
 * @param gpio :  requested gpio / pin
 *
 * return ALL_GOOD of succesfull (else error):
 *  *out = 1 or GPIOout if OUTPUT
 *  *out = 0 or GPIOin if input
 *
 * response looks like:
 * 0 [FF] header
 * 1 [02] length data
 * 2 [96] opcode
 * 3 [00] [00] repsonse code
 * 5 [01] always 1
 * 6 [01] input (0) or output (1)
*/
uint8_t RFID::getGPIODirection( uint8_t gpio, bool *out)
{
  if (gpio < GPI01 || gpio > LV4) return(ERROR_UNKNOWN_OPCODE);

  sendMessage(TMR_SR_OPCODE_SET_USER_GPIO_OUTPUTS, &gpio, 1);

  *out = (msg[6] == 1);

  return(msg[0]);
}

/** end section GPIO control *****************************************
