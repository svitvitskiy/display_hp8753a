#define PIN_LRFD 36       // Ready for data, active low
#define PIN_LDAV 38       // Data available, active low
#define PIN_DATA_LAST 39  // The first data pin

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LRFD, OUTPUT);
  digitalWrite(PIN_LRFD, 1);
  pinMode(PIN_LDAV, INPUT);
  for (int i = 0; i < 15; i++) {
    pinMode(PIN_DATA_LAST + i, INPUT);
  }  
}

void loop() {
  static char tbs[192];
  static unsigned short data_buf[16];
  static int data_pos = 0;
  
  // Before we begin, check the start conditions
  while(!digitalRead(PIN_LDAV));
  // We are ready to read
  digitalWrite(PIN_LRFD, 0);
  // Poll Data available pin
  while(digitalRead(PIN_LDAV));
  // Sample data
  //unsigned short pind = PIND;
  //unsigned short pinb = PINB;
  //unsigned short pinc = PINC;
  //unsigned short data = ((PINC & 0x1f) << 10) | ((PINB & 0x3f) << 4) | (PIND >> 4);
  unsigned short data = 0;
  for (int i = 0; i < 15; i++) {
    data = (data << 1) | digitalRead(PIN_DATA_LAST + i);
  }
  //data <<= 1;
   // Acknowledge
  digitalWrite(PIN_LRFD, 1);

  // Process the data
  data_buf[data_pos++] = data;
  if (data_pos == 16) {
    data_pos = 0;
    sprintf(tbs, "FFFF%04x%04x%04x%04x%04x%04x%04x%04x%04x%04x%04x%04x%04x%04x%04x%04x",
                 data_buf[0],   data_buf[1],  data_buf[2],  data_buf[3],  
                 data_buf[4],   data_buf[5],  data_buf[6],  data_buf[7],
                 data_buf[8],   data_buf[9], data_buf[10], data_buf[11],
                 data_buf[12], data_buf[13], data_buf[14], data_buf[15]);

    //sprintf(tbs, "0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x\n",
    //             data_buf[0],   data_buf[1],  data_buf[2],  data_buf[3],  
    //             data_buf[4],   data_buf[5],  data_buf[6],  data_buf[7],
    //             data_buf[8],   data_buf[9], data_buf[10], data_buf[11],
    //             data_buf[12], data_buf[13], data_buf[14], data_buf[15]);
    Serial.print(tbs);
  }
}
