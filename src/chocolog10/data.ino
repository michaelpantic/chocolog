/* Module that 
 * a) Updates the data info display page
 *    based on the data values written by the sensor module
 */

  
//                                               1234567890123456
const prog_uchar  data_header[] PROGMEM =       "Sensor Data:    ";
const prog_uchar  data_nodata[] PROGMEM =       "         No data";
byte data_subPageNum = 0;



/* Initialize data module
 */
void data_initialize()
{
  //nothing to do
}


/* Periodically update data page
 */
void data_loop()
{
   data_updatePage();
}

/* Periodically update data page
 */
void data_updatePage()
{
  //get page!
  char* page = pub_getEmptyPage(DATA_PAGE);
  
  //no buffer = nothing to do
  if(!page) return;
  
  //Calculate number of subpages needed in this module
  //One title page + 1 page per 4 sensors
  int numSubPages = (pub_sensor_count / 4) + 2;
  if(pub_sensor_count % 4 == 0) numSubPages--;
  
  //advance current subpage if a button has been pressed
  if(pub_retrieveButtonPress(DATA_PAGE) && pub_sensor_count != 0)
  {
    data_subPageNum = (data_subPageNum + 1) % numSubPages;
  }
  
  //if available sensors have been changed, reset to title page
  if(pub_sensor_changed) data_subPageNum = 0;
  
  
  if(data_subPageNum == 0)
  {
    //write title page
    //first line:
    memcpy_P(page, data_header, sizeof(prog_uchar)*16);
    
    //if there are no sensors, write "nodata" to second line
    if(pub_sensor_count == 0)
    {       
      memcpy_P(page+16, data_nodata, sizeof(prog_uchar)*16);
    }
  
  }
  else
  { 
    //write page with sensor data 
    //get first sensor on the current page
    int firstSensor = (data_subPageNum-1)*4;
    
    //write up to 4 sensor values. Starting from firstSensor on this page
    for(int i=0; i<4 && firstSensor+i < pub_sensor_count;i++)
    {
      //write new sensor info every 8 chars on the page (=> 4 sensors evenly distributed)
      data_writeSensor(page+i*8, firstSensor+i);
    }
    
  }

}


/* Writes a sensor value to the buffer
 *  example: 1=20.50   (sensor 1 = 20.50 degree)
 */
void data_writeSensor(char* pageBuffer, int numSensor)
{
  //write sensor number to first position
  convertToChar(pageBuffer, 1, numSensor, 9);
  
  //write = to second position
  pageBuffer[1] = '=';
  //pageBuffer[2] = ' '; //vorzeichen! TODO
  
  //write data of sensor numSensor to the next 5 characters with 2 decimal digits (format 00.00)
  convertFloatToChar(pageBuffer+2,5,pub_sensor_data[numSensor],2);

}
