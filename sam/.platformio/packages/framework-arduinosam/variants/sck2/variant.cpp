/*
  Copyright (c) 2014-2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
/*
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * + Pin number +  PIN   | SCK Label/Name  |  SERCOM         	| Comments
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * |            |        |                 |                 	|
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * | 0          |  PA11  | I2S_FS          |                	| EIC/EXTINT[11] ADC/AIN[19]           PTC/X[3] *SERCOM0/PAD[3]  SERCOM2/PAD[3]  TCC0/WO[3]  TCC1/WO[1]
 * | 1          |  PA10  | I2S_SCK         |                	| EIC/EXTINT[10] ADC/AIN[18]           PTC/X[2] *SERCOM0/PAD[2]                  TCC0/WO[2]  TCC1/WO[0]
 * | 2          |  PA14  | SPI_CS_SDcard   |                	| EIC/EXTINT[14]                                 SERCOM2/PAD[2]  SERCOM4/PAD[2]  TC3/WO[0]   TCC0/WO[4]
 * | 3          |  PA09  | FREE_1          |                	| EIC/EXTINT[9]  ADC/AIN[17]           PTC/X[1]  SERCOM0/PAD[1]  SERCOM2/PAD[1]  TCC0/WO[1]  TCC1/WO[3]
 * | 4          |  PA08  | FREE_2          |                	| EIC/NMI        ADC/AIN[16]           PTC/X[0]  SERCOM0/PAD[0]  SERCOM2/PAD[0]  TCC0/WO[0]  TCC1/WO[2]
 * | 5          |  PA15  | CD_SDcard       |                	| EIC/EXTINT[15]                                 SERCOM2/PAD[3]  SERCOM4/PAD[3]  TC3/WO[1]   TCC0/WO[5]
 * | 6          |  PA20  | LED_RED         |                	| EIC/EXTINT[4]                        PTC/X[8]  SERCOM5/PAD[2]  SERCOM3/PAD[2]              TCC0/WO[6]
 * | 7          |  PA21  | BUTTON          |                 	| EIC/EXTINT[5]                        PTC/X[9]  SERCOM5/PAD[3]  SERCOM3/PAD[3]              TCC0/WO[7]
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * |            |        |                 |                 	|
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * | 8          |  PA06  | PM_ENABLE       |                 	| EIC/EXTINT[6]  ADC/AIN[6]  AC/AIN[2] PTC/Y[4]  SERCOM0/PAD[2]                  TCC1/WO[0]
 * | 9          |  PA07  | I2S_SD          |                 	| EIC/EXTINT[7]  ADC/AIN[7]  AC/AIN[3] PTC/Y[5]  SERCOM0/PAD[3]                  TCC1/WO[1]
 * | 10         |  PA18  | LED_BLUE        |                 	| EIC/EXTINT[2]                        PTC/X[6]  SERCOM1/PAD[2]  SERCOM3/PAD[2]  TC3/WO[0]    TCC0/WO[2]
 * | 11         |  PA16  | SDA_B           | SERCOM1/PAD[0]  	| EIC/EXTINT[0]                        PTC/X[4]  SERCOM1/PAD[0]  SERCOM3/PAD[0]  TCC2/WO[0]   TCC0/WO[6]
 * | 12         |  PA19  | LED_GREEN       |                 	| EIC/EXTINT[3]                        PTC/X[7]  SERCOM1/PAD[3]  SERCOM3/PAD[3]  TC3/WO[1]    TCC0/WO[3]
 * | 13         |  PA17  | SCL_B           | SERCOM1/PAD[1]  	| EIC/EXTINT[1]                        PTC/X[5]  SERCOM1/PAD[1]  SERCOM3/PAD[1]  TCC2/WO[1]   TCC0/WO[7]
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * |            |        |                 |                 	|
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * | 14         |  PA02  | MEASURE_BATT    |                 	| EIC/EXTINT[2]  ADC/AIN[0]  DAC/VOUT  PTC/Y[0]
 * | 15         |  PB08  | CH_PD           |                 	| EIC/EXTINT[8]  ADC/AIN[2]            PTC/Y[14] SERCOM4/PAD[0]                  TC4/WO[0]
 * | 16         |  PB09  | GPIO0           |                 	| EIC/EXTINT[9]  ADC/AIN[3]            PTC/Y[15] SERCOM4/PAD[1]                  TC4/WO[1]
 * | 17         |  PA04  | TX_WIFI         | SERCOM0_ALT/PAD[0] | EIC/EXTINT[4]  ADC/AIN[4]  AC/AIN[0] PTC/Y[2]  SERCOM0/PAD[0]                  TCC0/WO[0]
 * | 18         |  PA05  | RX_WIFI         | SERCOM0_ALT/PAD[1] | EIC/EXTINT[5]  ADC/AIN[5]  AC/AIN[1] PTC/Y[5]  SERCOM0/PAD[1]                  TCC0/WO[1]
 * | 19         |  PB02  | TX_PMS          | SERCOM5_ALT/PAD[0] | EIC/EXTINT[2]  ADC/AIN[10]           PTC/Y[8]  SERCOM5/PAD[0]
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * |            |        |                 |                 	|
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * | 20         |  PA22  | SDA_A           | SERCOM3/PAD[0]  	| EIC/EXTINT[6]                        PTC/X[10]  SERCOM3/PAD[0] SERCOM5/PAD[0] TC4/WO[0] TCC0/WO[4]
 * | 21         |  PA23  | SCL_A           | SERCOM3/PAD[1]  	| EIC/EXTINT[7]                        PTC/X[11]  SERCOM3/PAD[1] SERCOM5/PAD[1] TC4/WO[1] TCC0/WO[5]
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * |            |        |                 |                 	|
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * | 22         |  PA12  | SPI_MISO        | SERCOM4/PAD[0]  	| EIC/EXTINT[12] SERCOM2/PAD[0]  SERCOM4/PAD[0] TCC2/WO[0] TCC0/WO[6]
 * | 23         |  PB10  | SPI_MOSI        | SERCOM4/PAD[2]  	| EIC/EXTINT[10]                 SERCOM4/PAD[2] TC5/WO[0]  TCC0/WO[4]
 * | 24         |  PB11  | SPI_SCK         | SERCOM4/PAD[3]  	| EIC/EXTINT[11]                 SERCOM4/PAD[3] TC5/WO[1]  TCC0/WO[5]
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * |            |        |                 |                 	|
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * | 25         |  PB03  | RX_PMS          | SERCOM5_ALT/PAD[1]	|
 * | 26         |  PA27  | USBLED          |                 	|
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * |            |        |                 |                 	|
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * | 27         |  PA28  | FREE_3          |                 	| EIC/EXTINT[8]
 * | 28         |  PA24  | TARGET_USB_N    |                 	| USB/DM
 * | 29         |  PA25  | TARGET_USB_P    |                 	| USB/DP
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * |            |        |                 |                 	|
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * | 30         |  PB22  | P_WIFI          |                 	| SERCOM5/PAD[2]
 * | 31         |  PB23  | P_GROOVE        |                 	| SERCOM5/PAD[3]
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 * | 38         |  PA13  | SPI_CS_Flash    |                 	| EIC/EXTINT[13] SERCOM2/PAD[1] SERCOM4/PAD[1] *TCC2/WO[1] TCC0/WO[7]
 * | 42         |  PA03  | INT_CHG         |                 	| EIC/EXTINT[3] [ADC|DAC]/VREFA ADC/AIN[1] PTC/Y[1]
 * +------------+--------+-----------------+--------------------+--------------------------------------------------------------------------------------------------------
 */


#include "variant.h"

/*
 * Pins descriptions
 */
const PinDescription g_APinDescription[]=
{

  { PORTA, 11, PIO_COM, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_11 },                                  // I2S_FS
  { PORTA, 10, PIO_COM, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_10 },                                  // I2S_SCK
  { PORTA, 14, PIO_DIGITAL, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_14 },                              // SPI_CS_SDcard
  { PORTA,  9, PIO_TIMER, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER), No_ADC_Channel, PWM0_CH1, TCC0_CH1, EXTERNAL_INT_NONE },      // FREE_1
  { PORTA,  8, PIO_TIMER, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER), No_ADC_Channel, PWM0_CH0, TCC0_CH0, EXTERNAL_INT_NONE },      // FREE_2
  { PORTA, 15, PIO_DIGITAL, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_15 },                              // CD_SDcard
  { PORTA, 20, PIO_TIMER_ALT, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER_ALT), No_ADC_Channel, PWM0_CH6, TCC0_CH6, EXTERNAL_INT_4 }, // LED_RED
  { PORTA, 21, PIO_DIGITAL, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_5 },                               // BUTTON
  { PORTA,  6, PIO_DIGITAL, PIN_ATTR_NONE, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, 				// PM_ENABLE
  { PORTA,  7, PIO_COM, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_7 },                                   // I2S_SD
  { PORTA, 18, PIO_TIMER, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER), No_ADC_Channel, PWM3_CH0, TC3_CH0, EXTERNAL_INT_2 },          // LED_BLUE
  { PORTA, 16, PIO_SERCOM, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_0 },                                // SDA_B: SERCOM1/PAD[0]
  { PORTA, 19, PIO_TIMER_ALT, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER_ALT), No_ADC_Channel, PWM0_CH3, TCC0_CH3, EXTERNAL_INT_3},  // LED_GREEN
  { PORTA, 17, PIO_SERCOM, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_1 },                                // SCL_B: SERCOM1/PAD[1]
  { PORTA,  2, PIO_ANALOG, PIN_ATTR_ANALOG, ADC_Channel0, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_2 },  					// MEASURE_BATT
  { PORTB,  8, PIO_DIGITAL, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_8 },                               // CH_PD
  { PORTB,  9, PIO_DIGITAL, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_9 },                               // GPIO0
  { PORTA,  4, PIO_SERCOM_ALT, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_4 },                            // TX_WIFI  SERCOM_ALT0/PAD[0] 
  { PORTA,  5, PIO_SERCOM_ALT, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_5 },                            // RX_WIFI  SERCOM_ALT0/PAD[1] 
  { PORTB,  2, PIO_SERCOM_ALT, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },                         // TX_PMS   SERCOM_ALT5/PAD[0]
  { PORTA, 22, PIO_SERCOM, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_6 },                                // SDA_A: SERCOM3/PAD[0]
  { PORTA, 23, PIO_SERCOM, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_7 },                                // SCL_A: SERCOM3/PAD[1]
  { PORTA, 12, PIO_SERCOM_ALT, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_12 },                           // SPI_MISO: SERCOM_ALT4/PAD[0]
  { PORTB, 10, PIO_SERCOM_ALT, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_10 },                           // SPI_MOSI: SERCOM_ALT4/PAD[2]
  { PORTB, 11, PIO_SERCOM_ALT, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_11 },                           // SPI_SCK: SERCOM_ALT4/PAD[3]
  { PORTB,  3, PIO_SERCOM_ALT, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },                         // RX_PMS   SERCOM_ALT5/PAD[1]
  { PORTA, 27, PIO_OUTPUT, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },                             // USBLED
  { PORTA, 28, PIO_DIGITAL, PIN_ATTR_NONE, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },                               // FREE_3
  { PORTA, 24, PIO_COM, PIN_ATTR_NONE, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },                                   // TARGET_USB_N
  { PORTA, 25, PIO_COM, PIN_ATTR_NONE, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },                                   // TARGET_USB_P
  { PORTB, 22, PIO_OUTPUT, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },                             // P_WIFI 
  { PORTB, 23, PIO_DIGITAL, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },                            // P_GROOVE

  // Arduino defined...                                                                                                                    *** REPEATED
  { PORTA, 22, PIO_SERCOM, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_6 },                                // *** SDA_A: SERCOM3/PAD[0]
  { PORTA, 23, PIO_SERCOM, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_7 },                                // *** SCL_A: SERCOM3/PAD[1]
  { PORTA, 19, PIO_PWM, (PIN_ATTR_DIGITAL|PIN_ATTR_PWM), No_ADC_Channel, PWM0_CH3, NOT_ON_TIMER, EXTERNAL_INT_NONE },                   // *** LED_GREEN
  { PORTA, 16, PIO_SERCOM, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_0 },                                // *** SDA_B: SERCOM1/PAD[0]
  { PORTA, 18, PIO_PWM, PIN_ATTR_NONE, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },                                   // *** LED_BLUE
  { PORTA, 17, PIO_SERCOM, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_1 },                                // *** SCL_B: SERCOM1/PAD[1]

  { PORTA, 13, PIO_DIGITAL, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_13 },                              // SPI_CS_Flash

  // Arduino defined...
  { PORTA, 21, PIO_DIGITAL, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_5 },                               // *** BUTTON
  { PORTA,  6, PIO_DIGITAL, PIN_ATTR_NONE, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, 				// *** PM_ENABLE
  { PORTA,  7, PIO_COM, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_7 },                                   // *** I2S_SD

  { PORTA, 3, PIO_DIGITAL, PIN_ATTR_DIGITAL, No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_3 },                                // *** INT_CHG

  // Arduino defined...
  { PORTA,  2, PIO_ANALOG, PIN_ATTR_ANALOG, ADC_Channel0, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_2 }, 					// *** MEASURE_BATT

} ;


const void* g_apTCInstances[TCC_INST_NUM+TC_INST_NUM]={ TCC0, TCC1, TCC2, TC3, TC4, TC5 } ;

// Multi-serial objects instantiation
SERCOM sercom0( SERCOM0 ) ;
SERCOM sercom1( SERCOM1 ) ;
SERCOM sercom2( SERCOM2 ) ;
SERCOM sercom3( SERCOM3 ) ;
SERCOM sercom4( SERCOM4 ) ;
SERCOM sercom5( SERCOM5 ) ;

Uart SerialESP( &sercom0, PIN_SERIAL_ESP_RX, PIN_SERIAL_ESP_TX, PAD_SERIAL_ESP_RX, PAD_SERIAL_ESP_TX ) ;
void SERCOM0_Handler()
{
  SerialESP.IrqHandler();
}
