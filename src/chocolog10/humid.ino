#define HUMID_IDLE 0
#define HUMID_MEASURING 1
#define HUMID_INTERVAL 2000 //once every 2 minute
SoftWire SWire = SoftWire();

int mode;
unsigned long humdity_last_read = 999999999;
uint8_t _i2caddr = 0x44;

void humid_init(){
  
   // Do softreset 
  SWire.beginTransmission(_i2caddr);
  SWire.write(SHT31_SOFTRESET >> 8);
  SWire.write(SHT31_SOFTRESET & 0xFF);
  SWire.endTransmission();  

  mode = HUMID_IDLE;
  pub_humidity=00.22;

}

void humid_loop(){
  if(mode == HUMID_IDLE && 
  pub_intervalElapsed(humdity_last_read, HUMID_INTERVAL)
    ){
   //Send measurement command
SWire.beginTransmission(_i2caddr);
 SWire.write(SHT31_MEAS_HIGHREP >> 8);
  SWire.write(SHT31_MEAS_HIGHREP & 0xFF);
  SWire.endTransmission();  
    humdity_last_read = millis();
    
        mode = HUMID_MEASURING;
        pub_humidity=11.11;
  }
  else if(mode == HUMID_MEASURING){
    //  delay(500);

    uint8_t readbuffer[6];
   //read 6 bytes
SWire.requestFrom(_i2caddr, (uint8_t)6);
  
  for (uint8_t i=0; i<6; i++) {
    readbuffer[i] = SWire.read();
  //  Serial.print("0x"); Serial.println(readbuffer[i], HEX);
  }
  
  //convert temp
  uint16_t ST, SRH;
  ST = readbuffer[0];
  ST <<= 8;
  ST |= readbuffer[1];
  
   SRH = readbuffer[3];
  SRH <<= 8;
  SRH |= readbuffer[4];

  
    double stemp = ST;
   stemp *= 175;
 	  stemp /= 0xffff;
	  stemp = -45 + stemp;
   
     double shum = SRH;
  shum *= 100;
  shum /= 0xFFFF;
  

    mode = HUMID_IDLE;
    pub_humidity = shum;
  }

}
