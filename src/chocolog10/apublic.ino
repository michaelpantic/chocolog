

/*********************************************************************/
/*  Public Variables available to all modules                        */
/*********************************************************************/
                         
long  pub_currentTimeStamp = 0; //seconds since 2014-01-01

byte  pub_sensor_data_valid = 0; //all pub_sensor variables is only to be used if this flag is set!
byte  pub_sensor_addresses[80];  //10 addresses * 8 bytes each (hardware addresses of the temp. sensors)
byte  pub_sensor_changed = 0;    //if since last loop any sensor config changed, this is set
byte  pub_sensor_count = 0;      //count of sensors (indicates number of valid pub_sensor_data and pub_sensor_addresses)
float pub_sensor_data[10];       //Holds sensor temperatures


byte pub_temp_cache[20];         //shared cache for various usages





/*********************************************************************/
/*  Public Functions available to all modules                        */
/*********************************************************************/


byte* pub_getBuffer()
{
  memset(pub_temp_cache,0,20);
  return pub_temp_cache;
}

inline byte pub_retrieveButtonPress(int pageNum)
{
  //redirect to UI
  return ui_retrieveButtonPress(pageNum);
}

inline char* pub_getEmptyPage(int pageNum)
{
  //redirect to UI
  return ui_getEmptyPage(pageNum);
}

/*********************************************************************/
/* Helper Functions                                                  */
/*********************************************************************/

/* Used for wait functions
 *  Returns true if millis() is larger than previousMillis+interval (in milliseconds)
 *  should be save for rollover of millis (happens after 47 days continous power)
 */
byte pub_intervalElapsed(unsigned long previousMillis, unsigned long interval)
{
   //save for rollover of millis.. taken from http://www.baldengineer.com/blog/2012/07/16/arduino-how-do-you-reset-millis/
   
   //get current time
   unsigned long currentMillis = millis();
 
   // How much time has passed, accounting for rollover with subtraction!
   return ((unsigned long)(currentMillis - previousMillis) >= interval);
}
   
   
   
/* Converts a byte of data into a HEX value
 *  writes 2 characters into buffer (without termination!!)
 */
void pub_convertByteToHex(char* buffer, unsigned char data)
{
  //temp with 3 chars (to hold \0)
  char temp[3]; 
  buffer[0] = '0';
  buffer[1] = '0';

  //convert and copy (overwrite previous written zeros where needed)
  itoa(data,temp,16);
  int len = strlen(temp);
  memcpy(buffer+(2-len),temp,len);
}



/* Converts a unix time stamp into a string
 *  (writes 20 characters into buffer): (without termination!!)
 *     YYYY-MM-DD hh:mm:ss\0 
 *    example: (christmas '13, 5:30 pm
 *    2013-12-25 17:30:00
 */
void pub_convertTimeToChar( char* buffer, long timestamp)
{
  
  tmElements_t clock_tm;
  breakTime(timestamp, clock_tm);

  //buld date string    
  convertToChar(buffer,    4, tmYearToCalendar(clock_tm.Year), 2050);
  buffer[4] = DATE_DELIMITER;
  convertToChar(buffer+5,  2, clock_tm.Month,   12);
  buffer[7] = DATE_DELIMITER;
  convertToChar(buffer+8,  2, clock_tm.Day,     31);
  buffer[10] = ' ';
  convertToChar(buffer+11, 2, clock_tm.Hour,    24);
  buffer[13] = TIME_DELIMITER;
  convertToChar(buffer+14, 2, clock_tm.Minute,  59);
  buffer[16] = TIME_DELIMITER;
  convertToChar(buffer+17, 2, clock_tm.Second,  59);
  buffer[19] = '\0'; 
}


/*
 * converts a integer to a given length char and check if its larger than maxExpected
 * output not defined for out of range values
 */
void convertToChar(char* buffer, byte numDigits, long number, long maxExpected)
{

  char temp[numDigits+1];
  //replace out of range numbers
  if(number<0L || number > maxExpected)
  {
    for(int i=0;i<numDigits;i++) {temp[i] = '#';}
    return;
  }

  //set leading zeros  
  for(int i=0;i<numDigits;i++) {buffer[i] = '0';}

  ltoa(number,temp,10);

  int len = strlen(temp);
  
  if((numDigits-len)<0) return;
  
  memcpy(buffer+(numDigits-len),temp,len);
}


/*
 * Writes a float into a char array
 * Example: value= 5.123
 *  numDigits = 7 (total number of chars to use)
 *  afterpoint = 1 (total number of digits after the point)
 * =
 *   00005.1
 *
 *  (7 chars in total, 1 after point)
 *  (partly from geeksforgeeks.org)
 */
void convertFloatToChar(char *res, byte numDigits, float n, byte afterpoint)
{
   
    //if number is negative, decrease number of total digits by one
    //and if possible after point digits as well
    //write -
    // and set n to abs(n)
    if(n < 0.0f) 
    {
       numDigits--;
       
       if(afterpoint>1) afterpoint--; 
       
       res[0]='-'; //set first character to -
       res++; //and advance pointer by one
       
       n = -n;
    }
    
     // Extract integer part
    int ipart = (int)n;
    
    // Extract floating part
    float fpart = n - (float)ipart;
    
    int integerPartDigits = numDigits-1-afterpoint;
    
    // convert integer part to string
    convertToChar(res, integerPartDigits, ipart, pow(10,integerPartDigits)-1);
 
    // check for display option after point
    if (afterpoint != 0)
    {
        res[integerPartDigits] = '.';  // add dot
 
        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter is needed
        // to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);
 
        convertToChar(res + integerPartDigits + 1,afterpoint,(int)fpart,pow(10,afterpoint)-1);
    }
}


