#include <Wire.h>
#include <DS1307RTC.h>

/* Module that communicates with a 
 * DS1307 RTC Clock and 
 * a) Updates the public timestamp
 * b) Updates the clock display page
 */


/* Initializes all hardware needed for 
 * Clock operation
 */
void clock_initialize()
{
  //nothing to do
}


/* Tests all hardware needed for 
 * Clock operation
 */
int clock_test()
{
  tmElements_t clock_tm;
  
  if(RTC.read(clock_tm))
  {
    return 0;
  }  
  else //if read did not succeed
  {
     if(RTC.chipPresent())
     {
       //if chip has been detected = clock is stopped
       return CLOCK_STOPPED;
     }
     else
     {
       //no chip detected (defective etc)
       return CLOCK_ERROR;
     }
  }
}

/* Periodical reads clock and  
 * updates clock page
 */
void clock_loop()
{
  //get time from rtc
  time_t currTime = RTC.get();
  if(currTime == 0) return; //no data
  
  //set public time stamp
  pub_currentTimeStamp =  currTime;
  
  clock_updatePage();  

}

/* Writes current date and time 
 * to clock page buffer
 */
void clock_updatePage()
{
  char* pageBuffer = pub_getEmptyPage(CLOCK_PAGE);

  if(pageBuffer)
  {  
    //convert current time to string in temporary buffer
    char* buffer = (char*)pub_getBuffer();
    pub_convertTimeToChar(buffer, pub_currentTimeStamp);
    
    //copy Date part to first line (right-aligned)
    memcpy(pageBuffer+6, buffer, sizeof(char)*10);
    
   //copy Time part to second line (right-aligned)
    memcpy(pageBuffer+23, buffer+10, sizeof(char)*9);
  }
}



