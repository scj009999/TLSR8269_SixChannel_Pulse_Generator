代码到 cc无需回复
/*
  pins_arduino.h - Pin definition functions for Arduino Telink TLSR8251F512ET32
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2007 David A. Mellis
  Modified for Telink TLSR8251F3ET / TLSR825x series by DroneProject

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA
*/

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

// ==============================================
// TLSR8251F512ET32 (TLSR512 F3ET) GPIO Mapping & Spec
// ✅ Package: QFN32 → Only 24 usable GPIOs:
// PA0~PA7 : GPIO 0 - 7   (8 pins)
// PB0~PB7 : GPIO 8 - 15  (8 pins)
// PC0~PC7 : GPIO 16 - 23 (8 pins)
// PD/PE/PF : NOT AVAILABLE (not bonded out in 32-pin package)
// ✅ Flash: 512KB, SRAM: 32KB, 1.9~3.3V, 3.3V IO only
// ✅ ADC: 6 channels (fixed on PB0~PB5)
// ✅ PWM: 6 hardware channels (PA0~PA3, PB0~PB1)
// ✅ Peripherals: 1xSPI, 1xI2C, 1xUART
// ==============================================

#define NUM_DIGITAL_PINS            24
#define NUM_ANALOG_INPUTS           6

// ✅ ADC fixed mapping: A0-A5 → PB0~PB5 → GPIO 8~13
#define analogInputToDigitalPin(p)  ((p < 6) ? (p + 8) : -1)

// ✅ Only these 6 pins have HARDWARE PWM
#define digitalPinHasPWM(p)  ( (p) == 0 || (p) == 1 || (p) == 2 || (p) == 3 || (p) == 8 || (p) == 9 )

// ------------------------------
// SPI (fixed pins)
// ------------------------------
static const uint8_t SS   = 7;   // PA7
static const uint8_t MOSI = 6;   // PA6
static const uint8_t MISO = 5;   // PA5
static const uint8_t SCK  = 4;   // PA4

static const uint8_t SPI_SS   = SS;
static const uint8_t SPI_MOSI = MOSI;
static const uint8_t SPI_MISO = MISO;
static const uint8_t SPI_SCK  = SCK;

// ------------------------------
// I2C (fixed pins)
// ------------------------------
static const uint8_t SDA = 14;   // PB6
static const uint8_t SCL = 15;   // PB7

static const uint8_t I2C_SDA = SDA;
static const uint8_t I2C_SCL = SCL;

// ------------------------------
// UART (fixed pins)
// ------------------------------
static const uint8_t RX = 16;    // PC0
static const uint8_t TX = 17;    // PC1

// ------------------------------
// LED & Analog Input
// ------------------------------
static const uint8_t LED_BUILTIN = 23;  // PC7 (most common on module)

static const uint8_t A0 = 8;    // PB0 - ADC0
static const uint8_t A1 = 9;    // PB1 - ADC1
static const uint8_t A2 = 10;   // PB2 - ADC2
static const uint8_t A3 = 11;   // PB3 - ADC3
static const uint8_t A4 = 12;   // PB4 - ADC4
static const uint8_t A5 = 13;   // PB5 - ADC5

// ------------------------------
// PWM channels
// ------------------------------
static const uint8_t PWM0 = 0;   // PA0
static const uint8_t PWM1 = 1;   // PA1
static const uint8_t PWM2 = 2;   // PA2
static const uint8_t PWM3 = 3;   // PA3
static const uint8_t PWM4 = 8;   // PB0
static const uint8_t PWM5 = 9;   // PB1

// ------------------------------
// ADC channels
// ------------------------------
#define ADC_CHANNEL_0  0
#define ADC_CHANNEL_1  1
#define ADC_CHANNEL_2  2
#define ADC_CHANNEL_3  3
#define ADC_CHANNEL_4  4
#define ADC_CHANNEL_5  5

// ------------------------------
// Timer definitions
// ------------------------------
#define TIMER0  0
#define TIMER1  1
#define TIMER2  2

// ------------------------------
// Interrupt → Pin mapping
// ------------------------------
#define EXTERNAL_INT_0  0   // PA0
#define EXTERNAL_INT_1  1   // PA1
#define EXTERNAL_INT_2  14  // PB6 (SDA)
#define EXTERNAL_INT_3  15  // PB7 (SCL)

#define EXTERNAL_INT_0_PIN  EXTERNAL_INT_0
#define EXTERNAL_INT_1_PIN  EXTERNAL_INT_1
#define EXTERNAL_INT_2_PIN  EXTERNAL_INT_2
#define EXTERNAL_INT_3_PIN  EXTERNAL_INT_3

#endif /* Pins_Arduino_h */