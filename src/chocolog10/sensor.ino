/*  Module that
 *   a) scans for available sensors 
 *   b) reads those sensors
 *   c) converts read data into float temperatures
 *
 *  Important: Because Bus operations (read, scan etc) take "a lot" of time, all operations
 *             are carried out piece by piece. For example:
 *             Bus scan: each call of the loop() function, search exactly for 1 new sensor, until all are found
 *             then, each call of the loop() reads exactly 1 sensor, until all are read
 *            
 *             This is done in order to keep the run time of the loop function as small as possible 
 */


#include <OneWire.h>

// Sensor Constants
#define SENSOR_BUS_PIN 4          //1 Wire Bus Pin
#define SENSOR_CONVERT_TIME 1000  // Time a sensor needs to digitalize temperature (750ms according to datasheet..better use 1000ms)
#define SENSOR_READ_INTERVAL 10000  // How often sensors are re-read
#define SENSOR_CMD_CONVERT 0x44   // Command to start temperature digitalization
#define SENSOR_CMD_READ    0xBE   // Command to read sensor memory with temperature

//module states
#define SENSOR_MODULE_DATAINVALID   0  //= No (valid) data present! Start Busscan!
#define SENSOR_MODULE_BUSSCAN 1        //= Busscan in progress
#define SENSOR_MODULE_BUSCONVERT 2     //= Busscan finished, start temperature digitalization!
#define SENSOR_MODULE_BUSWAIT 3        //= Wait for SENSOR_CONVERT_TIME to elapse
#define SENSOR_MODULE_BUSREAD 4        //= Reading Sensors in progress
#define SENSOR_MODULE_DATAVALID 5      //= All sensors read, everything valid! (number and address of sensors, sensor data!



//Display page texts
                              //0         1     ¦¦  2         3
                              //01234567890123456789012345678901
const prog_uchar sensor_pageDefaultText[] PROGMEM = "Sensor Info:           x DS18B20";

//module variables
byte sensor_moduleState = 0;       //current module state (see module states)
byte sensor_scanNum = 0;           //stores how many sensors have alread been found when in BUSSCAN mode
unsigned long sensor_startConversationTime;//stores timestamp of when convesation started(while in BUSWAIT mode)
byte sensor_nextSensorToRead=0;    //sets which sensor will be read enxt when in BUSREAD mode
unsigned long sensor_lastValidData;//stores timestamp of last time all sensors had valid data (needed in DATAVALID mode)


byte sensor_subPageNum = 0;  

OneWire sensor_bus(SENSOR_BUS_PIN);


void sensor_initialize()
{
  //nothing to initalize
}

/* Performs busscan for testing purposes and returns number of sensors found
 */
int sensor_test()
{
  //read all sensors address and return number of found sensors
   while(sensor_scanBus()); //search until all found
 
   return pub_sensor_count;
}

/*  Periodically read sensors and update page
 */
void sensor_loop()
{

  switch(sensor_moduleState)
  {
      case SENSOR_MODULE_DATAINVALID:
        // no valid data. Reset module variables
        // and set mode to BUSSCAN
        pub_sensor_data_valid = 0;
        sensor_scanNum = 0;
        pub_sensor_changed = 0;
        sensor_moduleState = SENSOR_MODULE_BUSSCAN;
        break;
        
      case SENSOR_MODULE_BUSSCAN:
        // Busscan in progress. while in this state,
        // each loop execution detects maximal one next sensor
        // if no new sensors have been detected (scanBus returns false), 
        // the scan is finished and next state is BUSCONVERT
        if(!sensor_scanBus())
        {
         sensor_moduleState = SENSOR_MODULE_BUSCONVERT; 
        }
        break;
        
      case SENSOR_MODULE_BUSCONVERT:
        //After Busscan, we know which sensors are present
        //Send them the convert command (= all sensors should read temperature and digitalize)
        //then set state to BUSWAIT
        sensor_busSendConvertCommand(); //send command
        sensor_nextSensorToRead=0;      //start reading with sensor 0
        sensor_startConversationTime = millis(); //conversation started!
          
        sensor_moduleState = SENSOR_MODULE_BUSWAIT;
        break;
        
      case SENSOR_MODULE_BUSWAIT:
        //Convert command has been sent. In this state
        //we only wait for the dataReadyTime to elapse.
        //if so, set state to BUSREAD        
        if(pub_intervalElapsed(sensor_startConversationTime, SENSOR_CONVERT_TIME))
        {
          sensor_moduleState = SENSOR_MODULE_BUSREAD;
        }
        break;
        
      case SENSOR_MODULE_BUSREAD:
          //Sensors are ready to be read.
          //read 1 sensor every loop() execution until
          //sensor_count has been reached
          if(sensor_nextSensorToRead<pub_sensor_count)
          {
           sensor_busReadSensorData(sensor_nextSensorToRead);
           sensor_nextSensorToRead++;
          }
          else //if all sensors have been read,
               //set data_valid = 1 and module state to DATAVALID 
          {
             sensor_moduleState = SENSOR_MODULE_DATAVALID;
             sensor_lastValidData = millis();
             pub_sensor_data_valid = 1;
          } 
      break;
      
      case SENSOR_MODULE_DATAVALID:
        //Data is valid. If READ_INTERVAL has been elapsed, start
        // again with DATAINVALID mode
        
        pub_sensor_changed = 0; //reset changed flag
        
        if(pub_intervalElapsed(sensor_lastValidData, SENSOR_READ_INTERVAL))
        {
          sensor_moduleState = SENSOR_MODULE_DATAINVALID;
        }
        break;
        
      default:
        sensor_moduleState = SENSOR_MODULE_DATAINVALID;
        break;        
    
  }


  //and in any state update display page
  sensor_updatePages();
  
}



/*  Searches for next sensor on bus
 *  if found, stores address, increases count and returns 1
 *  if not, finalizes scan and returns 0
 */
byte sensor_scanBus()
{
  uint8_t* cache = (uint8_t*)pub_getBuffer();
  
  //search if number of found sensors < 10
  // and store found address into cache
  if(sensor_bus.search(cache) && sensor_scanNum < 10) 
  {
    //for each byte
    for(int i=0;i<8;i++)
    {
      //compare to old address of this sensor number, set pub_sensor_changed if there is a difference
       pub_sensor_changed =  pub_sensor_changed | (pub_temp_cache[i] != pub_sensor_addresses[sensor_scanNum*8+i]);
       
       //and then overwrite old address of this sensor number in address array (pub_sensor_adresses)
       pub_sensor_addresses[sensor_scanNum*8+i] = pub_temp_cache[i];
    }
  
    //advance current sensor number
    sensor_scanNum++;
    
    return 1;
  }
  else //no more sensors found!
  {

    //set all remaining addresses (up to 10) to 0
    for(int i=sensor_scanNum*8;i<80;i++)
    {
      pub_sensor_addresses[i] = 0;
    }  
    
    //in any case, if number of sensors have changed, set pub_sensor_changed to 1
    if(pub_sensor_count != sensor_scanNum)
    {
      pub_sensor_changed = 1;
    }
    
    //update public sensor count variable
    pub_sensor_count = sensor_scanNum;
    
    //finish & reset bus
    sensor_bus.reset_search(); 
    
    return 0;
  }
  
}


/* Resets the bus and sends 
 * a convert (=read temp and digitalize)
 * command to ALL sensors on the bus
 */
void sensor_busSendConvertCommand()
{
    sensor_bus.reset();
    sensor_bus.skip(); //= don't select a specific address, write to all sensors
    sensor_bus.write(SENSOR_CMD_CONVERT);
}


/*  Read the value of the sensor with the given Number (numSensor)
 *  and store it in the pub_sensor_data array
 */
void sensor_busReadSensorData(int numSensor)
{

    sensor_bus.reset();
    sensor_bus.select(pub_sensor_addresses+numSensor*8); //select correct address (8byte length)
    sensor_bus.write(SENSOR_CMD_READ); //send command
    
    //read 9 bytes from sensor memory into cache (these 9 bytes contain the temp. value)
    byte* cache = pub_getBuffer();
    for ( int b = 0; b < 9; b++) {
      cache[b] = sensor_bus.read();
    }
    
    //convert to float and store
    pub_sensor_data[numSensor] =  sensor_convertBufferToFloat(pub_temp_cache);
}


/* Updates the sensor info page
 * Subpage 0 displays the number of sensors in total
 * Subpage 1-n displays 2 sensor addresses per page
 */
void sensor_updatePages()
{
  char* page = pub_getEmptyPage(SENSOR_PAGE);
  if(!page) return;
  
  
  //calculates number of needed pages according to num of sensors!
  int numSubPages = (pub_sensor_count/2 + pub_sensor_count % 2 + 1);
 
  //advance to next subpage if button has been pressed
  if(pub_retrieveButtonPress(SENSOR_PAGE) && pub_sensor_count != 0)
  {
    sensor_subPageNum = (sensor_subPageNum + 1) % numSubPages;
  }
  
  //if sensors have been changed, reset to page 0
  if(pub_sensor_changed) sensor_subPageNum = 0;
  
  
  //Display Page 0 (number of sensors)
  if(sensor_subPageNum == 0)
  {
   
    memcpy_P(page,sensor_pageDefaultText, sizeof(char)*PAGE_SIZE);  //copy base text
    convertToChar(page+20, 2, pub_sensor_count, 10); //copy number of sensors to the correct position in text
   
  }
  else
  {
    //else, display 2 Sensors per page
    int firstSensor = (sensor_subPageNum-1) * 2;

    for(int i=0; i<2 && firstSensor+i < pub_sensor_count; i++)
    {
      sensor_writeAddressInfo(page+i*16, firstSensor+i);
    }
  } 

}

/* Writes a sensor number and the corresponding address
 * to a char buffer. needs 16 chars!
 * example: 1234567890123456
 *          0:AABBCCDDEEFFGG
 */
void sensor_writeAddressInfo(char* buffer, int sensorNum)
{
    buffer[0]='0'+sensorNum; //write sensor number
    buffer[1]=':';
    for(int i=0; i<7;i++) { pub_convertByteToHex(buffer+2+i*2, pub_sensor_addresses[sensorNum*8+1+i]);} //write sensor address (only the upper 7 of 8 bytes)
}


/* Converts a DS18B20 memory read into a 
 * float temperature. 
 *  Taken from the OneWire Library examples!
 */
float sensor_convertBufferToFloat(byte* buffer)
{
  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (buffer[1] << 8) | buffer[0];
  int type_s = 0; //at the moment,always zero (only use DS18B20 sensors)
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (buffer[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - buffer[6];
    }
  } else {
    byte cfg = (buffer[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  
  return (float)raw / 16.0;

}


