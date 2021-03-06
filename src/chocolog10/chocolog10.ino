#define SDA_PORT PORTD
#define SDA_PIN 0
#define SCL_PORT PORTD
#define SCL_PIN 1
#define I2C_TIMEOUT 100
#define I2C_FASTMODE 1

#include <SoftWire.h>
#define SHT31_DEFAULT_ADDR    0x44
#define SHT31_MEAS_HIGHREP_STRETCH 0x2C06
#define SHT31_MEAS_MEDREP_STRETCH  0x2C0D
#define SHT31_MEAS_LOWREP_STRETCH  0x2C10
#define SHT31_MEAS_HIGHREP         0x2400
#define SHT31_MEAS_MEDREP          0x240B
#define SHT31_MEAS_LOWREP          0x2416
#define SHT31_READSTATUS           0xF32D
#define SHT31_CLEARSTATUS          0x3041
#define SHT31_SOFTRESET            0x30A2
#define SHT31_HEATEREN             0x306D
#define SHT31_HEATERDIS            0x3066

#include <Time.h>
#include<SdFat.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include "LowPower.h"

/*********************************************************************/
/*                       Definitions                                 */
/*********************************************************************/



#define DATE_DELIMITER '-'
#define TIME_DELIMITER ':'
#define UI_FLASH 501
#define UI_BLINK  500
#define UI_ON  1
#define UI_OFF  0


//UI definitions
#define PAGE_SIZE 33 //with termination char
#define CLOCK_PAGE 0
#define DATA_PAGE 1
#define RECORDING_PAGE 2
#define SENSOR_PAGE 3
#define TRANSMIT_PAGE 4
#define DIAG_PAGE 5


//Error codes
#define CLOCK_ERROR 1
#define CLOCK_STOPPED 2
#define SD_NOCARD 1

#define POWER_MODE_NORMAL 0
#define POWER_MODE_SAVE 1
#define POWER_MODE_SHUTDOWN 254

typedef unsigned char PROGMEM   prog_uchar;
typedef uint16_t PROGMEM   prog_uint16_t;

void startup_error(const __FlashStringHelper* message)
{
  
   char* pageBuffer = pub_getEmptyPage(0);  
   strcpy_P(pageBuffer, (const char*)message);

 
   Serial.print(message);

   ui_outputLoop();
   while(1){}
}

void shutdown_lowbat()
{
     // todo
       
}

void power_save()
{
  // sleep for a second
  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);  
  extern volatile unsigned long timer0_millis;
  noInterrupts();
  timer0_millis += 1000;
  interrupts();
}

void setup()
{ 

  pinMode(8, OUTPUT); 
  pinMode(10, OUTPUT);    


//  Serial.begin(115200);
  delay(500);
  pwrmgmt_initialize();
  ui_initialize();
  data_initialize();
  clock_initialize();
  humid_init();
  
  storage_initialize();
  sensor_initialize();
 
  ui_hw_test();
  switch(clock_test())
  {
    case 0: break; //everything ok
    
    case CLOCK_ERROR: startup_error(F("CLOCK ERROR"));
                      break;
                      
    case CLOCK_STOPPED: 
                        if(!clock_settime())
                        {
                          startup_error(F("CLOCK NOT SET"));
                        }
                        break;
  
  }
 
  
  switch(storage_test())
  {
    case 0: break;
    
    case SD_NOCARD: startup_error(F("NO SD CARD"));
                    break;
  
  }
  
  sensor_test();

}



void loop()
{
  

  //process any inputs by the buttons
  ui_inputLoop();
  
  //update current Time
  clock_loop();
 
  //read/scan sensors
  sensor_loop();
   
  //update storage and serial
  storage_loop();
  
  humid_loop();
  
  //update data display page
  data_loop();
  
  //update diag display page
  pwrmgmt_loop();
  
  //update LCD / Buttons
  ui_outputLoop(); 
  


  if(pwrmgmt_save_power())
  {
    power_save();
  }
}
