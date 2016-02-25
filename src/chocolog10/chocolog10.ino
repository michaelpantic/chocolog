#include <MemoryFree.h>
#include <Time.h>
#include<SdFat.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>

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

  
void startup_error(const __FlashStringHelper* message)
{
  
   char* pageBuffer = pub_getEmptyPage(0);  
   strcpy_P(pageBuffer, (const char*)message);

 
   Serial.print(message);

   ui_outputLoop();
   while(1){}
}

void setup()
{ 
  pinMode(8, OUTPUT); 
  pinMode(10, OUTPUT);  
 
  Serial.begin(115200);
  delay(500);
  
  ui_initialize();
  
  data_initialize();
  clock_initialize();

  
  storage_initialize();
  sensor_initialize();
  
  ui_hw_test();
   
  switch(clock_test())
  {
    case 0: break; //everything ok
    
    case CLOCK_ERROR: startup_error(F("CLOCK ERROR"));
                      break;
                      
    case CLOCK_STOPPED: startup_error(F("CLOCK NOT SET"));
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
  
  //update data display page
  data_loop();
  
  //update diag display page
  diag_loop();
  
  //update LCD / Buttons
  ui_outputLoop(); 
}