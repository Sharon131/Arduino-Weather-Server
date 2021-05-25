#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Ethernet.h>

#define FIXED_POINT_ACCURACY  1000
#define SHTC3_MAX_VALUE       65535

#define ZPH01_MSG_LEN     9
#define ZPH01_VALUES_NO   10

#define ZP01_A_PIN      4
#define ZP01_B_PIN      5

#define SHTC3_MSG_LEN   6

#define SHTC3_ADDRESS     0x70

uint8_t SHTC3_MEAS_COMM[SHTC3_MSG_LEN]= {0x7C, 0xA2};

char ZPH01_value[10];
char ZPH01_values[ZPH01_VALUES_NO][10];
uint8_t ZPH01_values_index = 0;

int ZP01_value;

uint32_t SHTC3_temp_raw;
uint32_t SHTC3_temp_value;
uint16_t SHTC3_hum_value;

SoftwareSerial ZPH01_serial(2, 3);  // RX, TX
uint8_t ZPH01_buffer[ZPH01_MSG_LEN];
uint8_t SHTC3_buffer[SHTC3_MSG_LEN];
char buffer[100];

uint16_t counter = 0;

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

IPAddress ip(192, 168, 31, 177);

EthernetServer server(80);

uint8_t ZPH01DV_checksum_correct(uint8_t* gotten_msg)
{
  uint8_t sum = 0;

  for (uint8_t i=1;i<ZPH01_MSG_LEN-1;i++) {
    sum += gotten_msg[i];
  }

  sum = (~sum) + 1;

  if (sum == gotten_msg[ZPH01_MSG_LEN-1]) {
    return 1;
  } else {
    return 0;
  }
}

uint32_t SHTC3_convertTemperature(uint16_t temp) {
//  return (temp*175.0/65535)-45;
  return temp*175*FIXED_POINT_ACCURACY/(SHTC3_MAX_VALUE) - 45*FIXED_POINT_ACCURACY;
}

void readSensors(void)
{
  if (ZPH01_serial.available() > 0)
  {
    ZPH01_serial.readBytes(ZPH01_buffer, ZPH01_MSG_LEN);
    if (ZPH01DV_checksum_correct(ZPH01_buffer))
    {
      sprintf(buffer, "Received messege from ZPH01: %u.%02u \r\n", ZPH01_buffer[3], ZPH01_buffer[4]);
      sprintf(ZPH01_value, "%u.%02u", ZPH01_buffer[3], ZPH01_buffer[4]);
      sprintf(ZPH01_values[ZPH01_values_index], "%u.%02u", ZPH01_buffer[3], ZPH01_buffer[4]);
      ZPH01_values_index = (ZPH01_values_index+1)%ZPH01_VALUES_NO;
      Serial.print(buffer);
    }
    else
    {
      Serial.print("Error in receiving data from ZPH01\r\n");
    }
  }

  if (counter > 20000) {
    // read from ZP01
    int val1 = digitalRead(ZP01_A_PIN);
    int val2 = digitalRead(ZP01_B_PIN);
    ZP01_value = (val1<<1)+val2;
    sprintf(buffer, "Received messege from ZP01: %u \r\n", ZP01_value);
    Serial.print(buffer);

    // read from SHTC1
    Wire.beginTransmission(SHTC3_ADDRESS);
    Wire.write(SHTC3_MEAS_COMM, 2);
    Wire.endTransmission();
    delay(100);
    Wire.requestFrom(SHTC3_ADDRESS, SHTC3_MSG_LEN);
    if (Wire.available() >= SHTC3_MSG_LEN) {
      for (int i=0;i<6;i++) {
        SHTC3_buffer[i] = Wire.read();  
      }
      SHTC3_temp_raw = (SHTC3_buffer[0]<<8)+SHTC3_buffer[1];
      SHTC3_temp_value = SHTC3_convertTemperature(SHTC3_temp_raw);
      SHTC3_hum_value = (SHTC3_buffer[3]<<8)+SHTC3_buffer[4];
      
      sprintf(buffer, "Received temp from SHTC3 raw: %u \r\n", (SHTC3_buffer[0]<<8)+SHTC3_buffer[1]);
      Serial.print(buffer);
      sprintf(buffer, "Received temp from SHTC3 conv: %u . %u \r\n", SHTC3_temp_value/FIXED_POINT_ACCURACY, SHTC3_temp_value%FIXED_POINT_ACCURACY);
      Serial.print(buffer);
      sprintf(buffer, "Received hum from SHTC3 raw: %u \r\n", (SHTC3_buffer[0]<<8)+SHTC3_buffer[1]);
      Serial.print(buffer);
    } 
    else {
      SHTC3_buffer[0] = 0;
//      sprintf(buffer, "Received messege from SHTC3: %u \r\n", (SHTC3_buffer[0]<<8)+SHTC3_buffer[1]);
      Serial.print("Error in receiving data from SHTC3.\r\n");
    }
    
    counter = 0;
  }
  counter++; 
}

void setup() {
  Serial.begin(115200);
//  while (!Serial) {
//    ; // wait for serial port to connect. Needed for Native USB only
//  }
  ZPH01_serial.begin(9600);

  pinMode(ZP01_A_PIN, INPUT);
  pinMode(ZP01_B_PIN, INPUT);

  Wire.begin(); 

  Ethernet.init(10);
  Ethernet.begin(mac, ip);
  
  Serial.println("Hello!");
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.");
    while (true) {
      delay(1); // do nothing
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}

void loop() {
//  uint8_t byte = ZPH01_serial.read();
  

  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      readSensors();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.print("ZPH01 read value: ");
          client.print(ZPH01_value);
          client.println("<br />");
          client.print("ZP01 read value: ");
          client.print(ZP01_value);
          client.println("<br />");
          client.print("SHTC3 read temperature raw: ");
          client.print(SHTC3_temp_raw);
          client.println("<br />");
          client.print("SHTC3 read temperature converted: ");
          client.print(SHTC3_temp_value/FIXED_POINT_ACCURACY);
          client.print(".");
          client.print(SHTC3_temp_value%FIXED_POINT_ACCURACY);
          client.println("<br/>");
          client.print("SHTC3 read humidity: ");
          client.print(SHTC3_hum_value);
          client.println("<br />");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
    Serial.println("client disconnected");
  }
  else
  {
    readSensors();
  }
}

//int main(void)
//{
//  Serial.begin(115200);
////  while (!Serial) {
////    ; // wait for serial port to connect. Needed for Native USB only
////  }
//  ZPH01_serial.begin(9600);
//
//  Serial.println("Hello!");
//  while(true) 
//  {
//    ZPH01_serial.readBytes(ZPH01_buffer, MESSAGE_LEN);
//    if (Serial.available())
//    {
//      Serial.write("Received message: ");
//      Serial.write(ZPH01_buffer, MESSAGE_LEN); 
//    }
//  }
//
//  return 0;
//}
