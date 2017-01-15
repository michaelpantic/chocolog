/*
 * EEPROM Write
 *
 * Stores values read from analog input 0 into the EEPROM.
 * These values will stay in the EEPROM when the board is
 * turned off and may be retrieved later by another sketch.
 */

#include <EEPROM.h>

/** the current address in the EEPROM (i.e. which byte we're going to write to next) **/
int addr = 1023;
int serialNumber = 4;
void setup() {
  Serial.begin(9600);
   Serial.println("Type w to start");
  char bla = Serial.read();
  while(bla!='w')
  {
    bla = Serial.read();
    }
   
   int oldval = EEPROM.read(addr);


  EEPROM.write(addr, serialNumber);
  Serial.print("Written serial num ");
  Serial.print(serialNumber);
  Serial.print(" into address ");
  Serial.println(addr);

  int val = EEPROM.read(addr);

  Serial.print("Old value = ");
  Serial.print(oldval);
  Serial.print(" / New value = ");
  Serial.println(val);

  Serial.println("Program ended.");
    
}

void loop() {
/*
  Serial.println("Type w to start");
  char bla = Serial.read();
  if(bla!='w'){
    return;}
  /*
 
  */
}

