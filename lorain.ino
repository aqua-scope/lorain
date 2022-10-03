#define PIN_REED    0x00
#define PIN_BUTTON  0x01
#define PIN_LEDR    0x0A
#define PIN_LEDG    0x09
#define PIN_NTC     PB3
#define PIN_NTC_VCC PB5
#define PIN_VOLTAGE PB4

#define LED_ON  HIGH
#define LED_OFF LOW
#define BUTTON_ON LOW
#define BUTTON_OFF HIGH

#define CONFIGSIZE 8
#define DEFAULT_INTERVAL  900 // 15 min
#define DEFAULT_HEARTBEAT 4 // 1h
#define DEFAULT_HEAVYRAIN 120 // 1 flip every 120 s => 30 f/h = 15 l/h

#define OTAA_DEVEUI   {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
#define OTAA_APPEUI   {0x0E, 0x0D, 0x0D, 0x01, 0x0E, 0x01, 0x02, 0x0E}
#define OTAA_APPKEY   {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3E}
 
bool                isLoRaWAN     =  false;
const char          compileDate[] = __DATE__ " " __TIME__;
const uint8_t       hwVersion = 0x05;
bool                heavyRain = false;
volatile uint16_t   flipCount = 0;
volatile bool       flipState = false;
uint32_t            lastFlip = 0;
uint32_t            confirmcount = 0;
uint16_t            uAs;
float               temperature;
uint16_t            voltage;
uint32_t            configs[CONFIGSIZE];
int                 heartbeat = 0;
bool                rx_done = false;

int switchMode(SERIAL_PORT port, char *cmd, stParam *param) {
  if (param->argc == 1) {     
    uint32_t mode = strtoul(param->argv[0],NULL,10);
    configs[7] = mode?0:1;
    writeConfigFlash();
    Serial.printf("Switch over to %s-Mode\n",mode?"LoRaWAN":"P2P");
    api.lorawan.nwm.set(mode); 
    writeConfigFlash();
  } else {
  return AT_PARAM_ERROR;
  }
  return AT_OK;
}

int defaultReset(SERIAL_PORT port, char *cmd, stParam *param) {
  Serial.println("Reset to Default");
  api.system.restoreDefault();
  return AT_OK;  
}

int setSerial(SERIAL_PORT port, char *cmd, stParam *param) {
  if (param->argc == 1) {     
    configs[6] = strtoul(param->argv[0],NULL,16);
    Serial.printf("Serial Number is 0x%04x\n",configs[6]);
  } else {
    return AT_PARAM_ERROR;
  }
  return AT_OK;
}

int cmdConfig(SERIAL_PORT port, char *cmd, stParam *param) {
    if (param->argc == 1 && !strcmp(param->argv[0], "?")) {
        Serial.printf("Configs %04x %04x %04x %04x %04x\n",configs[0],configs[1],configs[2],configs[3],configs[4]);
    } else if (param->argc == 1) {     
        String pv = String(param->argv[0]); 
        uint8_t p = strtoul(pv.substring(0,1).c_str(), NULL, 16);
        uint32_t v = strtoul(pv.substring(2).c_str(), NULL, 16);       
        configs[p]=v;
        Serial.printf("config %d:%04x\n",p,v);
        writeConfigFlash();
    } else {
        return AT_PARAM_ERROR;
    }
    return AT_OK;
}

void flip() { 
  uint32_t diff = millis() - lastFlip;
  if (diff < 4000) return;
  flipCount++;   
  if ((diff/1000) < configs[3] && !heavyRain) {
    heavyRain = true;
    Serial.println("HR Alarm Set");
    sendHeavyRainAlarm(true,diff);
  }
  else if ((diff/1000) > configs[3] && heavyRain) {
    sendHeavyRainAlarm(false,diff); 
    Serial.println("HR Alarm Cleared");
    heavyRain = false;      
  }   
  lastFlip = millis();    
  delay(200);
  uAs ++; // *0,1 mAs; (8000uA * 16 ms)
}

void buttonpress() { 
  detachInterrupt(PIN_BUTTON);  
  digitalWrite(PIN_LEDR, LED_ON); 
  Serial.println("button pressed");
  delay(1000);
  if(digitalRead(PIN_BUTTON) == BUTTON_ON && !api.lorawan.nwm.get()) {
    Serial.printf("Try LoRaWAN again: %s\r\n", api.lorawan.nwm.set(1) ? "Success" : "Fail");
  }
  if (isLoRaWAN) {
    sendHeartBeat();
    lorawanSend();
  }
  //delay(200);
  digitalWrite(PIN_LEDR, LED_OFF);
  attachInterrupt(PIN_BUTTON, buttonpress, BUTTON_ON);    
}

void signal(bool state) {
  digitalWrite(PIN_LEDR, LED_OFF);
  digitalWrite(PIN_LEDG, LED_OFF);
  for (int i=0;i<4;i++) {
    digitalWrite(state?PIN_LEDG:PIN_LEDR, LED_ON);
    api.system.sleep.all(50);
    digitalWrite(state?PIN_LEDG:PIN_LEDR, LED_OFF);
    api.system.sleep.all(200);
    }  
  digitalWrite(state?PIN_LEDG:PIN_LEDR, LED_OFF);   
  }

void setup() {
  Serial.begin(115200);
  Serial.printf("RAK Firmware Version: %s\r\n", api.system.firmwareVersion.get().c_str());
  pinMode(PIN_LEDG, OUTPUT);
  pinMode(PIN_LEDR, OUTPUT);
  pinMode(PIN_NTC_VCC, OUTPUT); 
  pinMode(PIN_NTC, INPUT);  
  pinMode(PIN_BUTTON, INPUT);  
  pinMode(PIN_VOLTAGE, INPUT);    
  pinMode(PIN_REED, INPUT);
  //analogReference(RAK_ADC_MODE_3_0);   
  if (api.lorawan.nwm.get()) { // LoRaWAN Mode
    digitalWrite(PIN_LEDG, LED_ON);
    digitalWrite(PIN_LEDR, LED_ON); 
    }
  api.system.atMode.add((char *)"CONFIG", (char *)"Get/Set Config Values", (char *)"CONFIG", cmdConfig, RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ); 
  api.system.atMode.add((char *)"RESET", (char *)"Factory Defaults", (char *)"RESET", defaultReset, RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ); 
  api.system.atMode.add((char *)"MODE", (char *)"Changes Lora Mode", (char *)"MODE", switchMode, RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);                         
  delay(3000); // emergency stop to input boot mode command
  readConfigFlash(); 
  loraSetup();
  attachInterrupt(PIN_REED, flip, FALLING);
  attachInterrupt(PIN_BUTTON, buttonpress, BUTTON_ON);  
  uAs = 1230; // *0.1 mAs setup 1,3 s @95 mA
}

void loop() {
  uint32_t diff = millis()/1000 - lastFlip;
  Serial.printf("Wakeup: time=%d flips=%d, count=%d,last=%d reed %d\n",millis()/1000,flipCount,heartbeat,lastFlip,digitalRead(PIN_REED));
  digitalWrite(PIN_NTC_VCC, HIGH); 
  delay(200);
  float r_ntc = (float)(configs[5] * 1000) * (1023.0/((float)analogRead(PIN_NTC))-1.0); // R7 is 100 kOhm    
  temperature  = (1 / ((log(r_ntc / 100000.0) / 4485.0) + (1.0 / (25.0 + 273.15))))- 273.15; 
  // NTCG164KF104FT1: b=4485/ r=100KOhm @ 25 Degree  
  voltage = analogRead(PIN_VOLTAGE)/10*75/100;
  digitalWrite(PIN_NTC_VCC, LOW);    
  uAs += configs[4] * 6/100; // * 0,1 mAs;
  if (++heartbeat >= configs[2] || flipCount || (heavyRain && diff > configs[3])) {
    confirmcount ++;
    uAs +=  57; // * 0,1 mAs (60 ms @ 95 mA)
    sendHeartBeat( );
    if (flipCount) flipCount=0; else heartbeat = 0;
    if (heavyRain && diff > configs[3]) {
      loraAddByteToBuffer(0x0b);
      loraAddByteToBuffer(0);  
      loraAddByteToBuffer(0x03);    
      loraAddWordToBuffer(0);
      heavyRain = false;       
    }
    if(isLoRaWAN) {
      if(confirmcount > 10) { // every 10th paket is confirmed to check join status
        api.lorawan.cfm.set(true); 
        confirmcount=0;     
      }
      else
        api.lorawan.cfm.set(false);
        lorawanSend();
    } 
  } 
  api.system.sleep.all(configs[4] * 1000);
}

void sendHeartBeat( ) {
  loraAddTwoByteToBuffer(0x06, 0x03); // Uptime
  loraAddWordToBuffer(millis()/1000/60/60/24);     
  loraAddTwoByteToBuffer(0x06, 0x01); // Temperature
  loraAddWordToBuffer((int16_t)(temperature*10)); 
  loraAddTwoByteToBuffer(0x06, 0x81); // rain
  loraAddWordToBuffer(flipCount); // count = 500 ml
  loraAddTwoByteToBuffer(0x12, voltage); // Battery
  loraAddWordToBuffer(uAs/3600);       
  }

bool readConfigFlash() {
  bool error= api.system.flash.get(0,(uint8_t *)configs,CONFIGSIZE*sizeof(uint32_t));
  if(configs[0]!=0xA5A5A5A5) {
    configs[0] = 0xA5A5A5A5;   // MAGIC 
    configs[1] = 1;                             // compatible to ESP32 System register       
    configs[2] = DEFAULT_HEARTBEAT;             // heartbeats as multiple of reports (default 4 = 1h,alternative 96 =1 d)
    configs[3] = DEFAULT_HEAVYRAIN;             // 1 flip every 120 s => 30 f/h = 15 l/h
    configs[4] = DEFAULT_INTERVAL;              // regular reporting in seconds (default 900 = 15 min)
    configs[5] = 100;                           // calibration of ntc     
    configs[6] = 0;            // id  
    configs[7] = 0;            // status         
    writeConfigFlash();
    return false;
  }
  return error;
}
 
void writeConfigFlash() {
  bool error= api.system.flash.set(0,(uint8_t *)configs,CONFIGSIZE*sizeof(uint32_t));
}

void readLoraKeys() {
  // OTAA Device EUI MSB first
  uint8_t node_device_eui[8] = OTAA_DEVEUI;
  // OTAA Application EUI MSB first
  uint8_t node_app_eui[8] = OTAA_APPEUI;
  // OTAA Application Key MSB first
  uint8_t node_app_key[16] = OTAA_APPKEY;
          
  api.lorawan.appeui.set(node_app_eui, 8);  
  api.lorawan.deui.set(node_device_eui, 8);
  api.lorawan.appkey.set(node_app_key, 16); 
}


// #####
// Lora Part
// #####
#define OTAA_BAND     (RAK_REGION_EU868)
#define OTAA_PERIOD   (20000)
#define FPORT           2
#define LORAWAN_BUFSIZE 50 

#define P2P_FREQUENCY   869525000
#define P2P_SF          12
#define P2P_BW          125
#define P2P_CR          1
#define P2P_TXPWR       22
#define P2P_PREAMPLE    8

#define BUILD_MONTH_IS_JAN (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_FEB (__DATE__[0] == 'F')
#define BUILD_MONTH_IS_MAR (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
#define BUILD_MONTH_IS_APR (__DATE__[0] == 'A' && __DATE__[1] == 'p')
#define BUILD_MONTH_IS_MAY (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
#define BUILD_MONTH_IS_JUN (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_JUL (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
#define BUILD_MONTH_IS_AUG (__DATE__[0] == 'A' && __DATE__[1] == 'u')
#define BUILD_MONTH_IS_SEP (__DATE__[0] == 'S')
#define BUILD_MONTH_IS_OCT (__DATE__[0] == 'O')
#define BUILD_MONTH_IS_NOV (__DATE__[0] == 'N')
#define BUILD_MONTH_IS_DEC (__DATE__[0] == 'D')

#define COMPUTE_BUILD_MONTH \
    ( \
        (BUILD_MONTH_IS_JAN) ?  1 : \
        (BUILD_MONTH_IS_FEB) ?  2 : \
        (BUILD_MONTH_IS_MAR) ?  3 : \
        (BUILD_MONTH_IS_APR) ?  4 : \
        (BUILD_MONTH_IS_MAY) ?  5 : \
        (BUILD_MONTH_IS_JUN) ?  6 : \
        (BUILD_MONTH_IS_JUL) ?  7 : \
        (BUILD_MONTH_IS_AUG) ?  8 : \
        (BUILD_MONTH_IS_SEP) ?  9 : \
        (BUILD_MONTH_IS_OCT) ? 10 : \
        (BUILD_MONTH_IS_NOV) ? 11 : \
        (BUILD_MONTH_IS_DEC) ? 12 : \
        /* error default */  99 \
    )
    
uint8_t         ubitxbuffer[25];
int             joinbackoff=10;
uint32_t        lBufPtr;
uint8_t         lBuf[LORAWAN_BUFSIZE];


//######################################################################
// Setup
//######################################################################

void        loraSetup() {
  uint8_t retcode=0;
  lBufPtr = 0; 
  if (api.lorawan.nwm.get()==0 && (configs[7] & 0x01) == 0) 
     Serial.printf("Set Node device work mode to LoRaWAN %s\r\n", api.lorawan.nwm.set(1) ? "Success" : "Fail");
  if (api.lorawan.nwm.get() == 1) {
    Serial.printf("Start LoRaWAN Mode\n");
    readLoraKeys();  // fills appkey,deveui,appeui
    api.lorawan.band.set(OTAA_BAND);
    api.lorawan.registerJoinCallback(joinCallback);
    api.lorawan.deviceClass.set(RAK_LORA_CLASS_A);
    api.lorawan.njm.set(RAK_LORA_OTAA); 
    retcode = api.lorawan.njs.get();
    api.lorawan.join();
    while(!isLoRaWAN && retcode == 0 && millis() < 20000) {
      delay(1000);
      retcode = api.lorawan.njs.get();
    };  
    if(retcode == 0) { // will reboot !!!!
      configs[7] |= 1;      
      signal(false);
//      api.system.sleep.all(24*60*60 * 1000);
      writeConfigFlash();
      Serial.printf("No LoRaWAN,Switch over to Ubilink\n");
      api.lorawan.nwm.set(0);
      return; //never reached
    }
    else
       signal(true);   
  }
}

void        lorawanSend() {
  if (lBufPtr==0) return;
  #ifdef DEBUG
  Serial.print("LoraWAN TX ");
  for (int i = 0; i < lBufPtr; i++) Serial.printf("%02x ", lBuf[i]);
  Serial.print("\r\n");  
  #endif //DEBUG
  api.lorawan.send(lBufPtr, (uint8_t *) lBuf, FPORT);
}

void        lorawanReceiveCallback(SERVICE_LORA_RECEIVE_T * data) {
  if (data->BufferSize > 0) {
    Serial.print("LoraWan RX ");
    for (int i = 0; i < data->BufferSize; i++)  
      Serial.printf("%02x ", data->Buffer[i]);
    Serial.print("\r\n");
  }
  processCmd(data->Buffer, data->BufferSize);
}

void        lorawanSendCallback(int32_t status) {
  if (status != 0) {
    Serial.printf("TX Failed. Join status CB: %d/%d\r\n", status, api.lorawan.njs.get());
  }
  loraResetBuffer();
}

void        joinCallback(int32_t status) {
  Serial.printf("Join status CB: %d\r\n", status);
  if (status == 0) {
    api.lorawan.adr.set(true); //set ADR on
    api.lorawan.rety.set(3); //retry times
    api.lorawan.cfm.set(1);  // confirmation
    api.lorawan.registerRecvCallback(lorawanReceiveCallback);
    api.lorawan.registerSendCallback(lorawanSendCallback); 
    sendFwID();
    sendHwID();
    isLoRaWAN = true;
    signal(true);   
  } else {      
    isLoRaWAN = false;    
    signal(false); 
  }  
}


//######################################################################
// ASAP  
//######################################################################

void        processCmd(uint8_t * buf, int buflen) {
  int cidx=0;
  int bcmd;
  while (cidx < buflen) {
    bcmd = buf[10+cidx++];
    if (bcmd == 0x03) sendHwID();
    else if (bcmd == 0x0a) sendFwID();
    else if (bcmd == 0x04) {
      uint8_t p = buf[10+cidx++];
      loraAddTwoByteToBuffer(0x04, p); 
      loraAddWordToBuffer(configs[p]); 
      Serial.printf("ASAP Config #%d ?\n",p);
      } 
    else if (bcmd == 0x014) {
     uint8_t p = buf[cidx++];
     uint16_t v = (buf[10+cidx++]<<8) + buf[10+cidx++];
     configs[p] = v;
     Serial.printf("ASAP Config #%d : 0x%04x\n",p,v);
     writeConfigFlash();
    }
    else if (bcmd == 0x00) return;
  }
}

void        sendHwID() {
  loraAddTwoByteToBuffer(0x03, (byte)0x05);
  loraAddWordToBuffer(0x0001); //  capabilities    
  }

void        sendFwID() { 
  uint32_t fwVersion = getFWVersion(); 
  loraAddByteToBuffer(0x0A);
  loraAddWordToBuffer(fwVersion >> 16);
  loraAddWordToBuffer(fwVersion & 0xffff);
  }

void        sendHeavyRainAlarm(bool state,uint32_t val) { 
  loraAddByteToBuffer(0x0b);
  loraAddByteToBuffer(state);  
  loraAddByteToBuffer(0x03);    
  loraAddWordToBuffer(val/1000);
  if(isLoRaWAN) {api.lorawan.cfm.set(true);lorawanSend();} 
  }

uint32_t    getFWVersion() {
    String cs(__DATE__);
    return cs.substring(9,11).toInt()*10000+COMPUTE_BUILD_MONTH*100+cs.substring(4,6).toInt();  
}

bool        loraAddByteToBuffer(byte x) {
  if (lBufPtr == LORAWAN_BUFSIZE) return false;
  lBuf[lBufPtr]=x;
  lBufPtr++;
  return true;
}

bool        loraAddTwoByteToBuffer(byte x, byte y) {
  if (lBufPtr == LORAWAN_BUFSIZE) return false;
  lBuf[lBufPtr]=x;
  lBuf[lBufPtr+1]=y;
  lBufPtr +=2;
  return true;  
}

bool        loraAddWordToBuffer(uint16_t x) {
  if (lBufPtr == LORAWAN_BUFSIZE) return false;
  lBuf[lBufPtr]=x>>8;
  lBuf[lBufPtr+1]=x & 0xff;
  lBufPtr+=2;
  return true;
}

void        loraResetBuffer() {
  lBufPtr = 0;
  memset(lBuf,0x00,LORAWAN_BUFSIZE);   
}
