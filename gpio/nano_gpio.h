/*********************************************************************
 * Section added for GPIO control
 * version 1.0 paulvha August 2019
 *
 * Be aware the pins can only handle 3.3V (as the rest of the Nano)
 *********************************************************************/
// pin select (as described on the simultaneous RFID board)
#define GPI01 1     // actually it could read LV1
#define LV2 2
#define LV3 3
#define LV4 4

// direction of the pin
#define GPIOout 1   // set as output
#define GPIOin 0    // set as input

// GPIO opcodes for Nano (do not change)
#define TMR_SR_OPCODE_GET_USER_GPIO_INPUTS    0x66
#define TMR_SR_OPCODE_SET_USER_GPIO_OUTPUTS   0x96

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
uint8_t setGPIO(uint8_t gpio, bool high);

/* read the GPIO level
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
 */
uint8_t getGPIO(uint8_t gpio, bool *state);

/* set GPIO direction
 *
 * @param gpio: between GPIO1 and LV4
 *
 * @param out :
 *  GPIOout(1) for output
 *  GPIOin (0) for input
 *
 * @value :
 *  in case of output (default = low/false)
 */
uint8_t setGPIODirection(uint8_t gpio, bool out, bool value = false);

/* get the GPIO direction
 *
 * @param gpio: between GPIO1 and LV4
 *
 * @param out :
 *  *out = 1 or GPIOout if output
 *  *out = 0 or GPIOin if input
 *
 * Return code :
 * OK = ALL_GOOD
 * else error (ignore *out)
 */
uint8_t getGPIODirection( uint8_t gpio, bool *out);

/** end section GPIO control *****************************************/
