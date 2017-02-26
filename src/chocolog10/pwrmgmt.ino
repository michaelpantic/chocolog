/* Module that 
 * handles all power mgmt issues
 * (reading vbat/reacting to low battery/sleep mode)
 */

#define PWRMGMT_BAT_MON_PIN A3
#define PWRMGMT_BAT_MON_INTERVAL 5000

#define PWRMGMT_BAT_SAVE_TIME 15000 // [ms]
#define PWRMGMT_BAT_SAVE_ON 4.00 //volts - below this voltage, power save mode is triggered
#define PWRMGMT_BAT_SAVE_OFF 4.15 //volts - above this voltage, power save mode is turned off
#define PWRMGMT_BAT_LOW 3.25 //volts - below this voltage, the logger shuts itself down!


const prog_uchar  vbatHeader[] PROGMEM =       "VBAT=";
const prog_uchar  serHeader[] PROGMEM =       "S/N=";
unsigned long vbat_last_read = 0;

/* Initialize diag module
 */
void pwrmgmt_initialize()
{
    pub_lastUserInput = millis();
}


void pwrmgmt_read_vbat()
{
  
  //only read vbat 
  if(pub_intervalElapsed(vbat_last_read, PWRMGMT_BAT_MON_INTERVAL))
  {
    pub_vbat = 0;
    // NOTE: DO NOT change number of samples without adjusting constant of scaling (0.0004887586 = 5.0/1023.0*0.1)
    for(int i=0; i<10; i++)
    {
      pub_vbat += analogRead(PWRMGMT_BAT_MON_PIN)*0.0004887586;
    }
    vbat_last_read = millis();
  }

}

void pwrmgmt_update_mode()
{ 
    // CAUTION! Setting mode_shutdown disables any user input, device has to be reset!
    if(pub_vbat < PWRMGMT_BAT_LOW)
    {
      pub_power_state = POWER_MODE_SHUTDOWN;  
      return;
    }

  
    if(pub_lastUserInput == 0){
      return;
    }
  
    // user input in last x seconds - no power save!
    if(!pub_intervalElapsed(pub_lastUserInput, PWRMGMT_BAT_SAVE_TIME))
    {
      pub_power_state = POWER_MODE_NORMAL;
      return;
    }
    
    // if we are not in power save mode,but voltage below power save threshold, turn save on
    if(pub_power_state == POWER_MODE_NORMAL && pub_vbat < PWRMGMT_BAT_SAVE_ON)
    {
       pub_power_state = POWER_MODE_SAVE;
       return;
    }

    // if we are in power save mode,but voltage above power save turn off threshold
    if(pub_power_state == POWER_MODE_SAVE && pub_vbat > PWRMGMT_BAT_SAVE_OFF)
    {
       pub_power_state = POWER_MODE_NORMAL;
       return;
    }
}


/* Periodically update diag page
 */
void pwrmgmt_loop()
{
  // update voltage
  pwrmgmt_read_vbat();
  
  // update power mgmt of device
  pwrmgmt_update_mode();
  
  // update status page if necessary
  char* page = pub_getEmptyPage(DIAG_PAGE);

  if(page) 
  {
   // write header from Progmem to page
   memcpy_P(page, vbatHeader, 5);
    
   // write number of millis since last loop
   convertFloatToChar(page+5, 4, pub_vbat, 2);

   memcpy_P(page+16, serHeader, 5);
   convertToChar(page+20, 2, pub_serial, 99);
  }
}

boolean pwrmgmt_save_power()
{
    return pub_power_state == POWER_MODE_SAVE;
}
