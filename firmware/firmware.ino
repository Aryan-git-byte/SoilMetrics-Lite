#include <SoftwareSerial.h>

// max485
#define de_pin 2
#define re_pin 3
#define ro_pin 4 
#define di_pin 5

// leds
#define activity_led 6
#define status_led 7

SoftwareSerial rs485(ro_pin, di_pin);

uint16_t crc16(uint8_t *data, uint8_t len){
  uint16_t crc=0xFFFF;
  for(uint8_t i=0;i<len;i++){
    crc^=data[i];
    for(uint8_t j=0;j<8;j++){
      if(crc&1){ crc>>=1; crc^=0xA001; }
      else crc>>=1;
    }
  }
  return crc;
}

void rs485Write(uint8_t *buf,uint8_t len){
  digitalWrite(de_pin,HIGH);
  digitalWrite(re_pin,HIGH);
  digitalWrite(activity_led,HIGH);

  delayMicroseconds(100);
  rs485.write(buf,len);
  rs485.flush();

  digitalWrite(de_pin,LOW);
  digitalWrite(re_pin,LOW);
}

bool readRegisters(uint16_t start,uint16_t count,uint8_t *resp,uint8_t expected){
  while(rs485.available()) rs485.read();

  uint8_t req[8]={
    0x01,0x03,
    (uint8_t)(start>>8),(uint8_t)start,
    (uint8_t)(count>>8),(uint8_t)count,
    0,0
  };

  uint16_t c=crc16(req,6);
  req[6]=c&0xFF;
  req[7]=c>>8;

  rs485Write(req,8);

  unsigned long t=millis();
  uint8_t i=0;

  while(millis()-t<1500){
    while(rs485.available() && i<expected){
      resp[i++]=rs485.read();
    }
    if(i>=expected) break;
  }

  digitalWrite(activity_led,LOW);

  if(i!=expected) return false;

  uint16_t rxcrc=resp[expected-2] | (resp[expected-1]<<8);
  if(rxcrc!=crc16(resp,expected-2)) return false;

  digitalWrite(status_led,HIGH);
  delay(30);
  digitalWrite(status_led,LOW);

  return true;
}

void setup(){
  Serial.begin(9600);
  rs485.begin(4800);

  pinMode(de_pin, OUTPUT);
  pinMode(re_pin, OUTPUT);
  pinMode(activity_led, OUTPUT);
  pinMode(status_led, OUTPUT);

  digitalWrite(de_pin, LOW);
  digitalWrite(re_pin, LOW);
  digitalWrite(activity_led, LOW);
  digitalWrite(status_led, LOW);

  Serial.println("SoilMetrics-Lite");
  Serial.println("Init............");
}

void loop(){

  uint8_t r1[13];
  uint8_t r2[11];

  if(readRegisters(0x0000,4,r1,13)){

    float moisture=((r1[3]<<8)|r1[4])/10.0;
    float temp=((int16_t)((r1[5]<<8)|r1[6]))/10.0;
    uint16_t ec=(r1[7]<<8)|r1[8];
    float ph=((r1[9]<<8)|r1[10])/10.0;

    Serial.print(F("moisture: ")); Serial.print(moisture); Serial.println(F(" %"));
    Serial.print(F("temperature: ")); Serial.print(temp); Serial.println(F(" C"));
    Serial.print(F("ec: ")); Serial.print(ec); Serial.println(F(" uS/cm"));
    Serial.print(F("ph: ")); Serial.println(ph);

  }else{
    Serial.println(F("Main sensor read failed"));
  }

  if(readRegisters(0x0004,3,r2,11)){

    uint16_t N=(r2[3]<<8)|r2[4];
    uint16_t P=(r2[5]<<8)|r2[6];
    uint16_t K=(r2[7]<<8)|r2[8];

    Serial.print(F("n: ")); Serial.print(N); Serial.println(F(" mg/kg"));
    Serial.print(F("p: ")); Serial.print(P); Serial.println(F(" mg/kg"));
    Serial.print(F("k: ")); Serial.print(K); Serial.println(F(" mg/kg"));

  }else{
    Serial.println(F("NPK read failed"));
  }

  Serial.println();
  delay(5000);
}