#define HUMID_IDLE 0
#define HUMID_MEASURING 1
#define HUMID_INTERVAL 60000 //once every minute
SoftWire SWire = SoftWire();

int mode;
unsigned long humdity_last_read = 0;
uint8_t _i2caddr = 0x44;

uint8_t crc8(const uint8_t *data, int len)
{
/*
*
 * CRC-8 formula from page 14 of SHT spec pdf
 *
 * Test data 0xBE, 0xEF should yield 0x92
 *
 * Initialization data 0xFF
 * Polynomial 0x31 (x8 + x5 +x4 +1)
 * Final XOR 0x00
 */

  const uint8_t POLYNOMIAL(0x31);
  uint8_t crc(0xFF);

  for ( int j = len; j; --j ) {
      crc ^= *data++;

      for ( int i = 8; i; --i ) {
	crc = ( crc & 0x80 )
	  ? (crc << 1) ^ POLYNOMIAL
	  : (crc << 1);
      }
  }
  return crc;
}


void humid_init(){
  
   // Do softreset 
  SWire.beginTransmission(_i2caddr);
  SWire.write(SHT31_SOFTRESET >> 8);
  SWire.write(SHT31_SOFTRESET & 0xFF);
  SWire.endTransmission();  
  mode = HUMID_IDLE;
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
    
       if(readbuffer[2] == crc8(readbuffer, 2)){
           pub_humidity = shum;
       }else{
           pub_humidity = -1.00;
       }

  }

}
