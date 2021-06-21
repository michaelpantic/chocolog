#include <SPI.h>
#include <LiquidCrystal.h>


//Definitions
#define PAGE_MEMORY_COUNT 6 //number of display pages
#define UI_BUTTON_DEBOUNCE 350

// PIN Configurations
#define UI_LCD_SLAVE_SELECT_PIN 10
#define UI_LCD_BACKLIGHT_PIN 7
#define UI_BLUE_BUTTON_LED_PIN A1
#define UI_RED_BUTTON_LED_PIN A0
#define UI_BLUE_BUTTON_SWI_PIN 2
#define UI_RED_BUTTON_SWI_PIN 3

//HW references
int16_t ui_redButtonLEDState = UI_OFF ;
int16_t ui_blueButtonLEDState = UI_OFF;    
unsigned long ui_redButtonLEDLastUpdate = 0;        
unsigned long ui_blueButtonLEDLastUpdate = 0;

//variables for use in interrupt handlers
volatile long ui_redButtonLastPressed = 0;
volatile long ui_blueButtonLastPressed = 0;

volatile int ui_redButtonState = 0;
volatile int ui_blueButtonState = 0;

LiquidCrystal ui_lcd(UI_LCD_SLAVE_SELECT_PIN);

uint8_t ui_lcdCurrentPage = 0;
byte ui_alert = 0;

//storage for pages 
byte ui_pages_buttonPressed[PAGE_MEMORY_COUNT];
char ui_page_buffer[PAGE_SIZE];



/*
 * Initializes all pins and interrupts
 */
void ui_initialize()
{
  //initialize LCD
  pinMode(UI_LCD_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(UI_LCD_BACKLIGHT_PIN, HIGH);

  pinMode(UI_LCD_SLAVE_SELECT_PIN, OUTPUT);
  ui_lcd.begin(16, 2);
  
  //set output modes for buttons LEDS
  // and input modes for button switches
  pinMode(UI_RED_BUTTON_LED_PIN, OUTPUT);
  pinMode(UI_BLUE_BUTTON_LED_PIN, OUTPUT);
  
  pinMode(UI_RED_BUTTON_SWI_PIN, INPUT_PULLUP);
  pinMode(UI_BLUE_BUTTON_SWI_PIN, INPUT_PULLUP);
  
  delay(10);
  
  //set up button interrupts (falling because PULLUP mode)
  attachInterrupt(0, ui_blueButtonPressed, FALLING); 
  attachInterrupt(1, ui_redButtonPressed, FALLING);
  
  //reset button states
  for(int i=0;i<PAGE_MEMORY_COUNT; i++)
  {
    ui_pages_buttonPressed[i] = 0;
  }
 
}


/*
 * Tests all available output hardware 
 * - Writes to every letter of the display
 * - activates both LEDs
 */
void ui_hw_test()
{
  ui_lcd.setCursor(0,0);
  
  // set all letters to "8"
  for(int i=0;i<32;i++)
  {
    ui_lcd.print("8");
    
    if(i==15)
    {  ui_lcd.setCursor(0,1);}
  }
  
  digitalWrite(UI_RED_BUTTON_LED_PIN, HIGH);
  digitalWrite(UI_BLUE_BUTTON_LED_PIN, HIGH);
  
  delay(500);
  
  //ui_lcd.clear();
 
  digitalWrite(UI_RED_BUTTON_LED_PIN, LOW);
  digitalWrite(UI_BLUE_BUTTON_LED_PIN, LOW);
  
}

/*
 * Runs the periodical loop for all 
 * Input devices (=> Buttons)
 * Must be run _before_ the output Loop
 */
void ui_inputLoop()
{
  ui_loopButtons();
}

/*
 * Runs the periodical loop for all 
 * Output devices (=> LED / LCD)
 * Must be run _after_ the input loop
 */
void ui_outputLoop()
{
  ui_loopLED();
  ui_loopLCD();
}

/*
 * Loop functions for buttons
 *  - Blue Button = Set button pressed = true for the current page
 *  - Red Button = Advance one page
 */
void ui_loopButtons()
{
  //set button pressed array to 1 for the current page  
  if(ui_blueButtonState)
  {
   ui_pages_buttonPressed[ui_lcdCurrentPage] = 1;
  }
  ui_blueButtonState=0; //reset
 
  //scroll to next page
  if(ui_redButtonState)
  {
    ui_lcdCurrentPage= (ui_lcdCurrentPage+1) % PAGE_MEMORY_COUNT;
 
  }
  ui_redButtonState = 0; //reset
 
}

/*
 * Loop functions for LEDs
 *  - Turn On / OFF / blink /flash if necessary
 *  - Has a lot of code duplicates.. could be refactored :-)
 */
void ui_loopLED()
{
 
  //Update RED led
  //if state is just ON or OFF, write this state to hardware
  if( ui_redButtonLEDState == UI_OFF || ui_redButtonLEDState == UI_ON)
  {
    digitalWrite(UI_RED_BUTTON_LED_PIN, ui_redButtonLEDState);
  }
  //if state is flash (= turn on once for a given amount of time)
  else if(ui_redButtonLEDState == UI_FLASH)
  {
    
    //get currentstate
    byte currentState = digitalRead(UI_RED_BUTTON_LED_PIN);
    
    //if LED is on and FLASH interval elapsed
    if(currentState && pub_intervalElapsed(ui_redButtonLEDLastUpdate, UI_FLASH))
    {
      //turn off and reset state to OFF
      digitalWrite(UI_RED_BUTTON_LED_PIN, 0);
      ui_redButtonLEDState = UI_OFF;
    }
    else if(currentState)
    {
       //if only LED is on, but timer not elapsed yet - do nothing
    }
    else //if LED is off t- turn it on and set timer
    {
      digitalWrite(UI_RED_BUTTON_LED_PIN, 1);
      ui_redButtonLEDLastUpdate = millis();
    }
    
  }
  else if(ui_redButtonLEDState == UI_BLINK)
  {
    //if blink interval elapsed, toggle state
    if(pub_intervalElapsed(ui_redButtonLEDLastUpdate, UI_BLINK))
    {
      digitalWrite(UI_RED_BUTTON_LED_PIN, !digitalRead(UI_RED_BUTTON_LED_PIN));    
      ui_redButtonLEDLastUpdate = millis();
    }
    
  }


  //Update BLUE led
  //if state is just ON or OFF, write this state to hardware
  if( ui_blueButtonLEDState == UI_OFF || ui_blueButtonLEDState == UI_ON)
  {
    digitalWrite(UI_BLUE_BUTTON_LED_PIN, ui_blueButtonLEDState);
  }
  //if state is flash (= turn on once for a given amount of time)
  else if(ui_blueButtonLEDState == UI_FLASH)
  {
    
    //get currentstate
    byte currentState = digitalRead(UI_BLUE_BUTTON_LED_PIN);
    
    //if LED is on and FLASH interval elapsed
    if(currentState && pub_intervalElapsed(ui_blueButtonLEDLastUpdate, UI_FLASH))
    {
      //turn off and reset state to OFF
      digitalWrite(UI_BLUE_BUTTON_LED_PIN, 0);
      ui_blueButtonLEDState = UI_OFF;
    }
    else if(currentState)
    {
       //if only LED is on, but timer not elapsed yet - do nothing
    }
    else //if LED is off t- turn it on and set timer
    {
      digitalWrite(UI_BLUE_BUTTON_LED_PIN, 1);
      ui_blueButtonLEDLastUpdate = millis();
    }
    
  }
  else if(ui_blueButtonLEDState == UI_BLINK)
  {
    //if blink interval elapsed, toggle state
    if(pub_intervalElapsed(ui_blueButtonLEDLastUpdate, UI_BLINK))
    {
      digitalWrite(UI_BLUE_BUTTON_LED_PIN, !digitalRead(UI_BLUE_BUTTON_LED_PIN));    
      ui_blueButtonLEDLastUpdate = millis();
    }
    
  }


  if(pub_power_state != POWER_MODE_NORMAL)
  {
    delay(5);
    digitalWrite(UI_BLUE_BUTTON_LED_PIN, LOW);  
    digitalWrite(UI_RED_BUTTON_LED_PIN, LOW);  
  }
   
}

/*
 * Updates the LCD based on the current data
 * ( currentPage + pageBuffer)
 */
void ui_loopLCD()
{
   //if power save mode, turn off
   if(pub_power_state == POWER_MODE_NORMAL)
   {
     digitalWrite(UI_LCD_BACKLIGHT_PIN, HIGH);
   }
   else
   {
     digitalWrite(UI_LCD_BACKLIGHT_PIN, LOW); 
     ui_lcd.clear();
     return;
   }
   
   //display correct page
   if(ui_lcdCurrentPage >= PAGE_MEMORY_COUNT || ui_lcdCurrentPage < 0)
   {
     return; //ignore invalid page num
   }
   
   //begin top left
   ui_lcd.setCursor(0,0);
   
   byte terminationReached=0;
   
   //for each byte in the buffer
   for(int i=0; i<PAGE_SIZE-1; i++)
   {
     
     //because its a 2x16 display, after 16 chars
     // switch to second line
     if(i==16)
     {
       ui_lcd.setCursor(0,1);
     }
     
     if(ui_page_buffer[i] == '\0')
     {
       terminationReached = true;
     }
     
     //if we encountered a binary 0, just print whitespaces
     if(terminationReached)
     {
      ui_lcd.print(' '); 
     }
     else //else, write one char of the buffer
     { 
       ui_lcd.print(ui_page_buffer[i]);
     }
     

    }

}


/*
 * Returns an empty page_buffer
 */
char* ui_getEmptyPage(int pageNum)
{
 
  //if requested page buffer is not current page, ignore
  if(pageNum != ui_lcdCurrentPage)
  {
    return 0;
  }
  
  //return the page buffer (the same for all pages)  
  for(int i=0;i< PAGE_SIZE; i++)
  {
    ui_page_buffer[i] = ' ';
  }
  
  ui_page_buffer[PAGE_SIZE-1]='\0';
  
  return ui_page_buffer;
}

/* Indicates for a specified page if the blue button
 * has been pressed on this page and
 * clears the button state
 */
byte ui_retrieveButtonPress(int pageNum)
{
  if(pageNum >= PAGE_MEMORY_COUNT || pageNum < 0)
   {
     return 0; //ignore invalid page num
   }
   
  //get pending button presses from array
  byte hasPress = ui_pages_buttonPressed[pageNum];
  ui_pages_buttonPressed[pageNum] = 0; //reset 
    
  return hasPress;
}

/*
 * Change the current page of the LCD
 */
void ui_setLCDPage(int pagenum)
{
  ui_lcdCurrentPage = pagenum;
}

/*
 * Change current mode of red LED
 */
void ui_setRedLED(int mode)
{
  ui_redButtonLEDState = mode;
}

/*
 * Change current mode of blue LED
 */
void ui_setBlueLED(int mode)
{
  ui_blueButtonLEDState = mode;
}

void ui_setAlert(int pageNum)
{
   ui_lcdCurrentPage = pageNum;
   ui_alert = 1; 
   ui_loopLCD();
}

void ui_clearAlert()
{
  ui_alert = 0;
}


/*
 * Interrupt handler for the red button
 * - ignores button presses in alert mode and 
 *  - if debounce time has not elapsed
 */
void ui_redButtonPressed()
{
  //if in alert mode - ignore button presses
  if(ui_alert) return;
   
  //if debouncing time has not elapsed - ignore
  if(!pub_intervalElapsed(ui_redButtonLastPressed, UI_BUTTON_DEBOUNCE))
  {
    return;
  }

  //else, set buttonstate to pressed
  ui_redButtonState = 1;
  ui_redButtonLastPressed = millis();
  pub_lastUserInput = ui_redButtonLastPressed;
  
}

/*
 * Writes a waiting banner to the buffer and returns 1
 * if more than 4 seconds elapsed since subPageLastChanged
 */
int ui_waitForOk(char* buffer, unsigned long subPageLastChanged)
{
    byte secondsSinceChange = (millis()-subPageLastChanged)/1000;
    
    for(int i=0;i<min(3,secondsSinceChange);i++)
    {
       buffer[i]='.'; //for any second, write a .
    }
    
    if(secondsSinceChange> 3)
    {
      buffer[3]= 'X'; //if more than 3 seconds elapsed, write a X
    }
    
    if(secondsSinceChange>4)
    {
           return 1; //return 1 if more than 4 seconds waited
    }
     
    return 0;
}

/*
 * Interrupt handler for the blue button
 * - ignores button presses in alert mode and 
 *  - if debounce time has not elapsed
 */
void ui_blueButtonPressed()
{
  //if in alert mode - ignore button presses
  if(ui_alert) return;
   
  //if debouncing time has not elapsed - ignore
  if(!pub_intervalElapsed(ui_blueButtonLastPressed, UI_BUTTON_DEBOUNCE))
  {
    return;
  }

  //else, set buttonstate to pressed
  ui_blueButtonState = 1;
  ui_blueButtonLastPressed = millis();
  pub_lastUserInput = ui_blueButtonLastPressed;
 
}
