#define STORAGE_SS 8                    //Chip select for SD Card (PIN 8 here)
#define STORAGE_DELIMITER '\t'          //column delimiter used in storage
#define STORAGE_STRING_ESCAPE '"'       //used to encapsulate strings
#define STORAGE_EEPROM_ADDR      0      //Address in EEPROM to store current recrod mode (uses 2 bytes)
#define STORAGE_EEPROM_ADDR_SERIAL      1023      //Address in EEPROM to store serial number (uses 1 bytes)
#define STORAGE_USB_LOG_INTERVAL 30000L //USB Log Interval in milliseconds


// Data used for the RECORDING page
                                                  //1234567890123456
const prog_uchar storage_rec_header[] PROGMEM =    "Recording:";
const prog_uchar storage_rec_set[] PROGMEM   =     "Set to";
byte storage_rec_subPage = 0;
unsigned long storage_rec_subPageLastChanged = 0;


//Data used for the USB Page  
                                              //1234567890123456
const prog_uchar storage_usb_header[] PROGMEM=  "USB-LINK: ";
const prog_uchar storage_usb_ena[] PROGMEM =    "Enable Log";
const prog_uchar storage_usb_disa[] PROGMEM =   "Dis";
const prog_uchar storage_usb_off[] PROGMEM =    "OFF";
const prog_uchar storage_usb_on[] PROGMEM =     "ON";
const prog_uchar storage_usb_upload[] PROGMEM = "UPLOAD";

byte storage_usb_subPage = 0;
unsigned long storage_usb_subPageLastChanged = 0;
SdFile entry; //file for directory listing/scroll

//Data used for the module
SdFat sd;
const prog_uint16_t storage_availableIntervals[] PROGMEM = {0, 1, 5, 10, 15, 30, 60, 6*60, 12*60}; //available record intervals
const int storage_numAvailableIntervals = 9;

//data needed for data recoding to SD
SdFile storage_logFile;
unsigned int storage_intervalMinutes = 0;
unsigned long storage_lastWritten = 0;

                            //123456789012
char* storage_logFileName = " No File    "; //should be 12 chars long (8.3 file format)
const prog_uchar storage_logFileExtension[] PROGMEM = ".txt";

//data needed for data logging to USB
unsigned long storage_usbLastLogged = 0;
byte storage_usbLogEnabled = 0;

/*
 * Initialize Module after power up
 */
void storage_initialize()
{
  //read previous config
   storage_readConfigEEPROM();  
}


/*
 * Tests if SD Card is present and initialized
 */
int storage_test()
{
  int sdState =  sd.begin(STORAGE_SS, SPI_HALF_SPEED);

  if(!sdState)
  {
    return SD_NOCARD;
  }

  return 0;
}


/*
 * Executed periodically (loop)
 */
void storage_loop()
{

  //update both pages
  storage_updatePage();
  storage_updateUSBPage();

  //update LED
  storage_loopRecLED();
  
  //if we have valid sensor data
  if(pub_sensor_data_valid)
  {
    //execute SD and log loop
    storage_loopSD();
    storage_loopLog();
  }
}

/*
 *  Turns LED on if file is open and off if file is closed
 */
void storage_loopRecLED()
{
   if(storage_logFile.isOpen())
   {
      ui_setRedLED(UI_ON); 
   }
   else
   {
     ui_setRedLED(UI_OFF);
   }
}


/*
 * Writes entries and headers to file if necessary
 */
void storage_loopSD()
{
  //check if recording is disabled
  if(storage_intervalMinutes == 0) 
  {
    //if recording is disabled, close file
    if(storage_logFile.isOpen())  storage_logFile.close();
    return;
  }
  else
  {
    //if recording is enabled, open file if necessary
    if(!storage_logFile.isOpen()) 
    {
      if(storage_lastWritten == 0) storage_updateFileName(); //if this is a new recording (lastWritten=0) set new filename
      storage_logFile.open(storage_logFileName, O_CREAT | O_WRITE );
      storage_logFile.seekEnd();
    }
  }


  //if sensors had been changed, write new header (only if this is NOT (lastWritten != 0) a new recording))
  if(pub_sensor_changed && storage_lastWritten != 0)
  {
    storage_writeHeader(&storage_logFile); 
  }

  //get interval in seconds
  long intervalSeconds = storage_intervalMinutes * 60L;  

  //if enough time elapsed since last written
  if(storage_lastWritten-(storage_lastWritten%intervalSeconds)+intervalSeconds <= pub_currentTimeStamp) //we don't need to use the pub_intervalElapsed funciton here, since we use seconds and not millis (no rollover)
  {
    storage_logFile.writeError = 0;
    //if no header has been written yet (first entry, write new header)
    if(storage_lastWritten == 0)
    {
      storage_writeHeader(&storage_logFile); 
    } 

    //write entry
    storage_writeEntry(&storage_logFile);
    
    if(storage_logFile.writeError == 1)
    {
       ui_setBlueLED(UI_BLINK); 
    }
    else
    {
        ui_setBlueLED(UI_FLASH); //flash blue LED to show activity
    }
    //update file and timestamps
    storage_logFile.sync();
    storage_lastWritten = pub_currentTimeStamp;
  }

}


/*
 * Writes entries and headers to USB(Serial) if necessary
 */
void storage_loopLog()
{
  
  //logging disabled, do nothing
  if(!storage_usbLogEnabled) return;

  //if sensors have changed and logging is in progress (lastLogged != 0) write new headers
  if(pub_sensor_changed && storage_usbLastLogged != 0)
  {
    storage_writeHeader(&Serial); 
  }

  //if its time to log
  if(pub_intervalElapsed(storage_usbLastLogged, STORAGE_USB_LOG_INTERVAL))
  { 
    //write header if new logging (==0)
    if(storage_usbLastLogged==0)
    {
      storage_writeHeader(&Serial); 
    }

    //write entry to Serial
    storage_writeEntry(&Serial);
    storage_usbLastLogged = millis();

  }  


}

/*
 * Writes current recording interval to EEPROM
 */
void storage_writeConfigEEPROM()
{
  EEPROM.write(STORAGE_EEPROM_ADDR ,storage_intervalMinutes & 0xFF); // write first byte
  EEPROM.write(STORAGE_EEPROM_ADDR+1 ,(storage_intervalMinutes>>8)& 0xFF); // write first byte

}


/*
 * Sets current recording interval from EEPROM
 */
void storage_readConfigEEPROM()
{

  // read serial number
  pub_serial = EEPROM.read(STORAGE_EEPROM_ADDR_SERIAL);

  storage_intervalMinutes = 0;
  byte val = EEPROM.read(STORAGE_EEPROM_ADDR);

  if(val == 255)
  {
    return; //no valid data
  }

  storage_intervalMinutes = val & 0xFF;

  val = EEPROM.read(STORAGE_EEPROM_ADDR+1);

  if(val == 255)
  {
    storage_intervalMinutes = 0;
    return;
  }

  storage_intervalMinutes += (val<<8) & 0xFFFF;


}



/* Prints the given interval in minutes to the buffer
 * Needs exactly 3 chars.
 * 0 is printed as OFF
 */
void storage_writeInterval(char* buffer, int minutes)
{

  if(minutes == 0)
  {
    buffer[0] = 'O';
    buffer[2] = buffer[1] = 'F';
  }
  else if(minutes % 60 == 0)
  {
    minutes = minutes / 60;
    convertToChar(buffer, 2, minutes, 99);
    buffer[2] = 'h';
  }
  else
  {
    convertToChar(buffer, 2, minutes, 99);
    buffer[2] = 'm';
  }
}


/*
 * Creates a new file Name in the 8.3 format.
 * Following schema applies:
 *  12345678
 *  YYMMDDSS.txt
 *  SS = logger serial
 *  example: A file create on the 2014-10-01 on logger 01 = 14100101.txt
 */
void storage_updateFileName()
{
  //convert time to char in buffer
  char* buffer = (char*)pub_getBuffer();                   // 0123456789012345678
  pub_convertTimeToChar(buffer, pub_currentTimeStamp); // format: YYYY-MM-DD HH:MM:SS


  memcpy(storage_logFileName, buffer+2 ,sizeof(char)*2); //YY
  memcpy(storage_logFileName+2, buffer+5 ,sizeof(char)*2); //MM
  memcpy(storage_logFileName+4, buffer+8 ,sizeof(char)*2); //DD
  convertToChar(storage_logFileName+6, 2, pub_serial, 99);
  
  memcpy_P(storage_logFileName+8, storage_logFileExtension, sizeof(char)*4); //file extension
}




/*
 * Writes a header to the given Printable Object
 */
void storage_writeHeader(Print* s)
{
  
  //Write timestamp headers
  s->println();
  s->print(STORAGE_STRING_ESCAPE);
  s->print("Timestamp");
  s->print(STORAGE_STRING_ESCAPE);
  s->print(STORAGE_DELIMITER);
  s->print(STORAGE_STRING_ESCAPE);
  s->print("Realtime");
  s->print(STORAGE_STRING_ESCAPE);
  s->print(STORAGE_DELIMITER);
  s->print(STORAGE_STRING_ESCAPE);
  s->print("RH");
  s->print(STORAGE_STRING_ESCAPE);


  //write sensor headers
  for(int i=0;i<pub_sensor_count;i++)
  {
    s->print(STORAGE_DELIMITER);
    s->print(STORAGE_STRING_ESCAPE);
    s->print(i); //sensor number
    
    //sensor address (7 upper bytes)
    s->print(" (");

    char* buffer = (char*)pub_getBuffer();
    for(int b=0; b<7; b++) { 
      pub_convertByteToHex(buffer+b*2, pub_sensor_addresses[i*8+1+b]);
    }
    buffer[14] = '\0';

    s->print(buffer);
    s->print(")");
    s->print(STORAGE_STRING_ESCAPE);
  }
  s->println();

}


/*
 * Writes the current entry to the given Printable Object
 */
void storage_writeEntry(Print* s)
{
  //write timestamp
  s->print(pub_currentTimeStamp);
  s->print(STORAGE_DELIMITER);  

  //time string
  char* buffer = (char*)pub_getBuffer();
  pub_convertTimeToChar(buffer, pub_currentTimeStamp);
  s->print(STORAGE_STRING_ESCAPE);
  s->print(buffer);
  s->print(STORAGE_STRING_ESCAPE);
  s->print(STORAGE_DELIMITER);
  s->print(pub_humidity);

  //Sensor data
  for(int i=0;i<pub_sensor_count;i++)
  {
    s->print(STORAGE_DELIMITER);
    s->print(pub_sensor_data[i]);
  }

  s->println();
}


/*
 * Updates the main storage page
 * First Line: "Recording: " + current Interval
 * Second LIne: Last written time (Page 0)
 * Second Line: Current File Name (Page 1)
 * Second LIne: "Set to "+Interval (Page 2+) (menu to change record frequency)
 */
void storage_updatePage()
{
  
  char* page = pub_getEmptyPage(RECORDING_PAGE);

  if(!page) //if page has been changed, reset to subpage 0
  {
    storage_rec_subPageLastChanged = 0;
    storage_rec_subPage = 0;
    return;
  }

  //2 fixed pages (last written value, current filename + all intervals)
  int numPages = storage_numAvailableIntervals+2;

  //update current subpage if button has been pressed (according to limits)
  if(pub_retrieveButtonPress(RECORDING_PAGE))
  {
    storage_rec_subPageLastChanged=millis();
    storage_rec_subPage = (storage_rec_subPage + 1) % numPages;
  }


  //write first  line (header text and current interval)
  memcpy_P(page, storage_rec_header, sizeof(char)*10);
  storage_writeInterval(page+12, storage_intervalMinutes);

  //second line depends on the current subpage
  if(storage_rec_subPage == 0)
  {
    //Page 0 - write last written time
    if(storage_lastWritten != 0)
    {
      char* buffer = (char*)pub_getBuffer();
      pub_convertTimeToChar(buffer, storage_lastWritten);
      memcpy(page+18, buffer+5, sizeof(char)*14);
    }
  }
  else if(storage_rec_subPage == 1)
  {
    //Page 1 - write current LogFile Name 
    memcpy(page+16+4, storage_logFileName, sizeof(char)*12);    
  }
  else
  {
    //Page 2+ - display menu to change record interval
    memcpy_P(page+16+1, storage_rec_set, sizeof(char)*6);

    //get interval for this page from PROGMEM
    int tempVal;
    memcpy_P(&tempVal, storage_availableIntervals+storage_rec_subPage-2, sizeof(int));
    storage_writeInterval(page+16+8, tempVal);    

    //wait for ok for the currently display interval
    byte ok = ui_waitForOk(page+16+12, storage_rec_subPageLastChanged);

    if(ok)
    {
      //update configuration if interval is aproved
      storage_lastWritten = 0;
      storage_intervalMinutes = tempVal;
      storage_writeConfigEEPROM(); 
      storage_rec_subPage = 0;
    }    

  }


}

/* Scrolls through the root directory
 * Using the "entry" file variable
 * Returns 0 if end of directory is reached
 *
 */
byte storage_sdEntryAdvance()
{

  //open next file that is not a directory
   char* cache = (char*)pub_getBuffer();
   
   do
   {
    if(entry.isOpen()) entry.close();
    entry.openNext(sd.vwd(),O_READ);
    if(entry.isOpen()) entry.getFilename(cache);

   }
   while(entry.isOpen() && 
                     (entry.isDir())
          );
     
     //no file found - end of dir reached
    if(!entry.isOpen()) 
    {
      sd.vwd()->rewind();
      return 0;
    }
      
    return 1;
}

/*
 * Updates the main storage page
 * First Line: "USB-LINK: " + State (OFF, ON, UPLOAD)
 * Second LIne: Enable/Disable USB Logging (Page 0)
 * Second Line: Scrolls trough files on SD Card to select file to upload (Page 1+)
 */
void storage_updateUSBPage()
{
  
  // The "entry" file is used to scroll trought the sd card

  
  char* page = pub_getEmptyPage(TRANSMIT_PAGE);

  if(!page) //if page has been changed, reset to subpage 0
  {
    storage_usb_subPageLastChanged = 0;
    storage_usb_subPage = 0;
    if(entry.isOpen())  entry.close(); //close any files that might be open
    return;
  }


  //on button press, increase subpage and if page >= 2, get next filename on sd card
  if(pub_retrieveButtonPress(TRANSMIT_PAGE))
  {
    storage_usb_subPageLastChanged=millis();
    storage_usb_subPage++;


    if (storage_usb_subPage >= 2)
    {
      //get next file on sd card or reset to page 0 if end of directory reached
      if(!storage_sdEntryAdvance())
      {
       storage_usb_subPage = 0; 
      }
    }

  }

  //write first  line
  memcpy_P(page, storage_usb_header, 10);
  if(storage_usbLogEnabled)
  {
   memcpy_P(page+12, storage_usb_on, sizeof(char)*2);
  }
  else
  {
   memcpy_P(page+12, storage_usb_off, sizeof(char)*3);
  }

  //if subpage 1, display menu to enable disable usb logging
  if(storage_usb_subPage == 1)
  {
    if(storage_usbLogEnabled)
    {
      memcpy_P(page+16+2, storage_usb_ena, 10);
      memcpy_P(page+16+1, storage_usb_disa, 3);
    } 
    else
    {
      memcpy_P(page+16+1, storage_usb_ena, 10);
    }

    //if this subpage is ok, toggle storage_usbLogEnabled
    byte ok = ui_waitForOk(page+16+12, storage_usb_subPageLastChanged);
    if(ok)
    {
      storage_usbLogEnabled = !storage_usbLogEnabled;
      storage_usbLastLogged = 0;
      storage_usb_subPage = 0;
      storage_usb_subPageLastChanged = 0;
    }

  }
  else if(storage_usb_subPage > 1) //on any other page, display filenames 
  {
    if(!entry.isOpen()) return; //no file ot display
    
    char* buffer = (char*)pub_getBuffer();
    entry.getFilename(buffer); //writes the name (12 chars into the buffer)
    byte len = strlen(buffer);
    memcpy(page+16,buffer,len);
    
    
    //wait if this file is chosen
    byte ok = ui_waitForOk(page+16+12, storage_usb_subPageLastChanged);
    
    //if so, transmit file and return to page 0
    if(ok)
    {
      memcpy_P(page+10,storage_usb_upload,sizeof(char)*6);
      storage_transmitFile(&entry,page+16);       
      entry.close();
      storage_usb_subPage = 0;
      storage_usb_subPageLastChanged = 0;

     }
  }
}

/*
 * !! BLOCKS THE ENTIRE DEVICE !!
 *  Outputs the given file to the serial port and updates a progressbar in the given buffer (16 chars)
 */
void storage_transmitFile(SdFile* f, char* pageBuffer)
{
  //get temporary storage
  char* cache = (char*)pub_getBuffer();
  entry.getFilename(cache); //writes
    
    
  //clear pagebuffer
  for(int i=0;i<16;i++){pageBuffer[i]=' ';}

  //block ui on this page and update
  ui_setAlert(TRANSMIT_PAGE);
  
  Serial.println();
  Serial.println();
  //output filename
  for(int i=0;i<100; i++)
  {
    Serial.print(">");
    
    if(i==5)
    {
       Serial.print(' ');
       Serial.print(cache);
       Serial.print(' ');
    }
  }
  Serial.println();
  
  float fileSize = (float)f->fileSize()/16.0f;
  int progress = 0;
  
   //output file
   while (f->available()) 
   {
    Serial.write(f->read());

    //update progress
    int newprogress =  f->curPosition()/fileSize;

    if(newprogress > progress)
    {
      for(int i=0; i<=progress; i++)
      {
        pageBuffer[i] = '=';
      }
       
      //block and update
      ui_setAlert(TRANSMIT_PAGE);
    }
         
     progress = newprogress;
   }
  
  Serial.println();
  Serial.println();
  //output filename
  for(int i=0;i<100; i++)
  {
    Serial.print("<");
    
    if(i==95)
    {
       Serial.print(' ');
       Serial.print(cache);
       Serial.print(' ');
    }
  }
  Serial.println();
  Serial.println();
  
  //clear block
  ui_clearAlert();
     
}

