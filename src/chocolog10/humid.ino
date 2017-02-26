#define HUMID_IDLE 0
#define HUMID_MEASURING 1
#define HUMID_INTERVAL 120000 //once every 2 minute

int mode;
unsigned long humdity_last_read = 0;

void humid_init(){
  i2c_init();
  mode = HUMID_IDLE;

}

void humid_loop(){
  if(mode == HUMID_IDLE && 
  pub_intervalElapsed(humdity_last_read, HUMID_INTERVAL)
    ){
    //single shot high repeatablity without clock stretching
    byte cmd_lsb = 0x00;
    byte cmd_msb = 0x24;
  
    i2c_start(0x44 | I2C_WRITE);
    i2c_write(cmd_msb);
    i2c_write(cmd_lsb);
    mode = HUMID_MEASURING;
    humdity_last_read = millis();
  }
  else if(mode == HUMID_MEASURING){
    if(!i2c_start(0x44 | I2C_READ)){
      return; //no data yet
    }
    i2c_read(false); //ignore tmp MSB
    i2c_read(false); //ignore tmp LSB
    i2c_read(false); //ignore tmp CRC
    byte humid_msb = i2c_read(false);
    byte humid_lsb = i2c_read(false);
    byte humid_crc = i2c_read(true);
    pub_humidity = 100 * (float)(humid_msb<<8 & humid_lsb)/65535; 
    i2c_stop();
    mode == HUMID_IDLE;
  }

}
