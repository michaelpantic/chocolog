/* Module that 
 * a) Writes diagnostic data to a page (Loop duration and Memory count)
 */

//                                             12345678
const prog_uchar  genHeader[] PROGMEM =       "Loop   =";
const prog_uchar  memHeader[] PROGMEM =       "Memory =";
long diag_lastLoop = 0;


/* Periodically update diag page
 */
void diag_loop()
{

  char* page = pub_getEmptyPage(DIAG_PAGE);

  if(page) 
  {
 
   //write header from Progmem to page
   memcpy_P(page, genHeader, 8);
    
   //write number of millis since last loop
   convertToChar(page+9, 7, millis()-diag_lastLoop, 999999);
    
   //write second line header to page line 2    
   memcpy_P(page+16, memHeader, 8);
    
   //get free memory count & write to page
   int mem = freeMemory();
   convertToChar(page+25, 7, mem, 999999);
  
  }
  
  //update lastLoop termination time
  diag_lastLoop = millis();

}
