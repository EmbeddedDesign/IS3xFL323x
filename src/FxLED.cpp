/*
  FxLED.cpp - Library for interfacing with the ISSI IS3xFL323x Family of FxLED Drivers
  http://ams.issi.com/US/product-analog-fxled-driver.shtml
  Created by stephen.harper@noctivore.com
*/
 
// Include headers
#include "FxLED.h"

#ifdef USE_AVR
ArduinoI2C i2c;
#endif
I2CInterface* IS3xFL323x::pI2CInterface = &i2c;
 
// Constructor
IS3xFL323x::IS3xFL323x()
{
}

void IS3xFL323x::begin(uint8_t AD)
{
  AD_BITS = AD;
  sendCommandAuto(LED_CONTROL_REG_START, LED_CONTROL_REG_START+0x23, 0xFF);
  // pI2CInterface->beginTransmission(ADDRESS_REG+AD_BITS);
  // pI2CInterface->write(LED_CONTROL_REG_START);
  // for(uint8_t i=LED_CONTROL_REG_START;i<=LED_CONTROL_REG_START+0x23;i++)
  // {
  //   // sendCommand(i,0xFF); // Enable all LED Control Registers
  //   pI2CInterface->write(0xFF);
  // }
  sendCommandAuto(0x01, 0x24, 0x00);
  // pI2CInterface->write(0x01);
  // for(uint8_t i=0x01;i<=0x24;i++)
  // {
  //   // sendCommand(i,0x00); // Set all PWM Registers to 0x00
  //   pI2CInterface->write(0x00);
  // }

  sendCommand(PWM_UPDATE_REG,0x00); // Write PWM Update Registers
  sendCommand(OUTPUT_FREQ_REG,0x01); // Set frequency to 22KHz
  sendCommand(SHUTDOWN_REG,0x01); // Normal operation
}

// Reset device and all registers to default values
void IS3xFL323x::reset() {
  sendCommand(RESET_REG,0x00); // Write Reset Register
}

// Software shutdown enable
void IS3xFL323x::shutdown(bool enable) {
  sendCommand(SHUTDOWN_REG,(enable) ? (0x00) : (0x01));
}

// // #ifdef HAS_FREQ_CONTROL
// // Set the frequency of PWM output
// void IS3xFL323x::setPWMFrequency(uint8_t frequency) {
//   if(frequency == FREQ_3KHZ || frequency == FREQ_22KHZ) {
//     sendCommand(OUTPUT_FREQ_REG, frequency); // Set frequency
//   }
// }
// // #else
// // #pragma GCC error "Function not available for chipset"
// // #endif

// Update the PWM Control registers
void IS3xFL323x::update() {
  sendCommand(PWM_UPDATE_REG,0x00);//update PWM & control registers
}

// Update the PWM Control registers
void IS3xFL323x::update(FxRGB leds) {
  // for(uint8_t i=0; i<sizeof(leds)/sizeof(leds[0]); i++) {
  for(uint8_t i=0; i<12; i++) {
    setPWM(leds[i].rChannel, leds[i].pixel.r, 0);
    setPWM(leds[i].gChannel, leds[i].pixel.g, 0);
    setPWM(leds[i].bChannel, leds[i].pixel.b, 0);
  }
  sendCommand(PWM_UPDATE_REG,0x00);//update PWM & control registers
}

// Turn on a specific channel, full brightness
void IS3xFL323x::on(uint8_t channel) {
  sendCommand(channel+0x25,0xFF); // Enable channel
  setPWM(channel);
}

// Turn off a specific channel
void IS3xFL323x::off(uint8_t channel) {
  sendCommand(channel+0x25,0x00); // Disable channel
}

// Set Channel PWM Frequency
void IS3xFL323x::setPWM(uint8_t channel, uint8_t value, double delay) {
  sendCommand(channel,value);//set PWM
  _delay_ms(delay);
}

// Clear all channels
void IS3xFL323x::clear() {
  for(uint8_t i=0x01;i<=0x24;i++)
  {
    sendCommand(i,0x00); // Set all PWM Registers to 0x00
  }
}

// Fade all LEDs off
void IS3xFL323x::fadeAll(double delay) {
  for (int8_t i=63; i>=0; i--)//all LED breath falling
  {
    for(uint8_t j=0x01; j<=0x24; j++)
    {
      sendCommand(j, pgm_read_byte(&PWM_Gamma64[i]));//set all PWM
    }
    // sendCommandAuto(0x01, 0x24, pgm_read_byte(&PWM_Gamma64[i]));
    update();
    _delay_ms(delay);
  }
}

// Display a digit on a seven segment display
void IS3xFL323x::displayDigit(FxSevenSegDisplay digits, uint8_t digit, char value, bool flip) {
  uint8_t segment;
  uint8_t flipped[8] = {3, 4, 5, 0, 1, 2, 6, 7};

  for(uint8_t i=0; i<8; i++)
  {
    // Handle rearranging of segments to flip display
    if(flip)
    {
      segment = digits[digit][flipped[i]];
    }
    else
    {
      segment = digits[digit][i];
    }
    
    // If corresponding bit of value is 1 and segment is not disabled (0)
    if(((value >> i) & 1) && (segment != 0))
    {
      sendCommand(segment+0x25,0xFF); // Enable channel
      setPWM(segment, 0x3F, 0);
    }
    else if(segment != 0)
    {
      sendCommand(segment+0x25,0x00); // Disable channel
    }
  }
}

// #ifdef HAS_FREQ_CONTROL
// Display the time on a 4-digit seven segment display
void IS3xFL323x::displayTime(FxSevenSegDisplay digits, int hour, int minute, uint8_t format, bool flip, bool leadingZero) {
  // Call twoDigitDisplay helper function
  twoDigitDisplay(digits, false, hour, format, flip, leadingZero);
  twoDigitDisplay(digits, true, minute, format, flip, leadingZero);
}
// #else
// #pragma GCC error "Function not available for chipset"
// #endif
 
// Sends the I2C command to read data
// Returns the status of the transmission
uint8_t IS3xFL323x::sendCommand(uint8_t Reg_Add, uint8_t Reg_Dat)
{
  // Begin transmission
  pI2CInterface->beginTransmission(ADDRESS_REG+AD_BITS);
  pI2CInterface->write(Reg_Add);
  pI2CInterface->write(Reg_Dat);
  // End the transmission (stop transmitting)
  // return the status of the transmission
  // 0:success
  // 1:data too long to fit in transmit buffer
  // 2:received NACK on transmit of address
  // 3:received NACK on transmit of data
  // 4:other error
  return pI2CInterface->endTransmission();
}

uint8_t IS3xFL323x::sendCommandAuto(uint8_t Reg_Add_Start, uint8_t Reg_Add_Stop, uint8_t Reg_Dat)
{
  pI2CInterface->beginTransmission(ADDRESS_REG+AD_BITS);
  pI2CInterface->write(Reg_Add_Start);
  for(uint8_t i=Reg_Add_Start; i<=Reg_Add_Stop; i++)
  {
  	pI2CInterface->write(Reg_Dat);
  }
  // End the transmission (stop transmitting)
  // return the status of the transmission
  // 0:success
  // 1:data too long to fit in transmit buffer
  // 2:received NACK on transmit of address
  // 3:received NACK on transmit of data
  // 4:other error
  return pI2CInterface->endTransmission();
}

// Helper function for two digit displays
void IS3xFL323x::twoDigitDisplay(FxSevenSegDisplay digits, bool isRightmostDigits, int value, uint8_t format, bool flip, bool leadingZero) {
  uint8_t FIRST_DIGIT = 0;
  uint8_t SECOND_DIGIT = 1;
  uint8_t THIRD_DIGIT = 2;
  uint8_t FOURTH_DIGIT = 3;

  if(isRightmostDigits)
  {
    FIRST_DIGIT = 2;
    SECOND_DIGIT = 3;
    THIRD_DIGIT = 0;
    FOURTH_DIGIT = 1;
  }
  
  // If value is two digits in specifed number format
  if(value>(format-1))
  {
    displayDigit(digits, (flip) ? (FOURTH_DIGIT) : (FIRST_DIGIT), sevenSegCharMap[value/format], flip);
    displayDigit(digits, (flip) ? (THIRD_DIGIT) : (SECOND_DIGIT), sevenSegCharMap[value%format], flip);
  }
  // If value is one digit in specifed number format
  else
  {
    // If leading zeros are enabled
    if (leadingZero)
    {
      // Display a zero for left digit
      displayDigit(digits, (flip) ? (FOURTH_DIGIT) : (FIRST_DIGIT), sevenSegCharMap[0], flip);
    }
    else
    {
      // Display nothing
      displayDigit(digits, (flip) ? (FOURTH_DIGIT) : (FIRST_DIGIT), 0x00, flip);
    }
    // Display right digit
    displayDigit(digits, (flip) ? (THIRD_DIGIT) : (SECOND_DIGIT), sevenSegCharMap[value], flip);
  }
}

void FL3236A::setPWMFrequency(uint8_t frequency) {
  if(frequency == FREQ_3KHZ || frequency == FREQ_22KHZ) {
    sendCommand(OUTPUT_FREQ_REG, frequency); // Set frequency
  }
}