//#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

#define FIXED_POINT_ACCURACY  1000
#define SHTC3_MAX_VALUE       65535

#define ZPH01_MSG_LEN       9
#define ZPH01_VALUES_NO     10


#define ZP01_A_PIN          4
#define ZP01_B_PIN          5

#define SHTC3_MSG_LEN       6

#define SHTC3_ADDRESS       0x70

#define FILE_MAX_SAMPLES_NO  10

uint8_t SHTC3_MEAS_COMM[SHTC3_MSG_LEN]= {0x7C, 0xA2};

int ZP01_value;

SoftwareSerial ZPH01_serial(2, 3);  // RX, TX
//uint8_t SHTC3_buffer[SHTC3_MSG_LEN];
char buffer[30];

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

IPAddress ip(192, 168, 31, 177);
EthernetServer server(80);

char* fileName = "sensdata/000_ZPH01.txt";
uint8_t fileIndex = 0;

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

void readSensors(void)
{
  if (ZPH01_serial.available() > 0)
  {
    uint8_t ZPH01_buffer[ZPH01_MSG_LEN];
    ZPH01_serial.readBytes(ZPH01_buffer, ZPH01_MSG_LEN);
    if (ZPH01DV_checksum_correct(ZPH01_buffer))
    {
      char ZPH01_value[6];
      sprintf(ZPH01_value, "%u.%02u", ZPH01_buffer[3], ZPH01_buffer[4]);

      fileName[11] = fileIndex+48;

      File file = SD.open(fileName, O_WRITE | O_CREAT | O_TRUNC);
      if (file) {
        file.println(ZPH01_value);
        file.close();
        fileIndex = (fileIndex+1)%FILE_MAX_SAMPLES_NO;
      } else {
        Serial.print("SD write error");
        Serial.println(fileName);
      }
      
      // read from ZP01
//      int val1 = digitalRead(ZP01_A_PIN);
//      int val2 = digitalRead(ZP01_B_PIN);
//      ZP01_value = (val1<<1)+val2;
//      sprintf(buffer, "Received messege from ZP01: %u \r\n", ZP01_value);
//      Serial.print(buffer);

      // read from SHTC1
//    Wire.beginTransmission(SHTC3_ADDRESS);
//    Wire.write(SHTC3_MEAS_COMM, 2);
//    Wire.endTransmission();
//    delay(100);
//    Wire.requestFrom(SHTC3_ADDRESS, SHTC3_MSG_LEN);
//    if (Wire.available() >= SHTC3_MSG_LEN) {
//      for (int i=0;i<6;i++) {
//        SHTC3_buffer[i] = Wire.read();  
//      }
////      SHTC3_temp_raw = (SHTC3_buffer[0]<<8)+SHTC3_buffer[1];
////      SHTC3_temp_value = SHTC3_convertTemperature(SHTC3_temp_raw);
////      SHTC3_hum_value = (SHTC3_buffer[3]<<8)+SHTC3_buffer[4];
////      
////      sprintf(buffer, "Received temp SHTC3 raw: %u \r\n", (SHTC3_buffer[0]<<8)+SHTC3_buffer[1]);
////      Serial.print(buffer);
////      sprintf(buffer, "Received temp SHTC3 conv: %u . %u \r\n", SHTC3_temp_value/FIXED_POINT_ACCURACY, SHTC3_temp_value%FIXED_POINT_ACCURACY);
////      Serial.print(buffer);
////      sprintf(buffer, "Received hum SHTC3 raw: %u \r\n", (SHTC3_buffer[0]<<8)+SHTC3_buffer[1]);
////      Serial.print(buffer);
//    } 
//    else {
//      SHTC3_buffer[0] = 0;
////      sprintf(buffer, "Received messege from SHTC3: %u \r\n", (SHTC3_buffer[0]<<8)+SHTC3_buffer[1]);
//      Serial.print("Error receiving from SHTC3.\r\n");
//    }
    
    }
    else
    {
      Serial.print("Error receiving ZPH01 data\r\n");
    }
  }
}

void setup() {
  Serial.begin(115200);
//  while (!Serial) {
//    ; // wait for serial port to connect. Needed for Native USB only
//  }
  ZPH01_serial.begin(9600);

  pinMode(ZP01_A_PIN, INPUT);
  pinMode(ZP01_B_PIN, INPUT);

//  Wire.begin(); 

  Ethernet.init(10);
  Ethernet.begin(mac, ip);
  
  Serial.println("Hello!");
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("ETH no shield");
    while (true) {
      delay(1);
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("ETH no cable");
  }
  if (!SD.begin(4)) {
    Serial.println("SD init error");
    while (1);
  }
  
  server.begin();
  Serial.println("Ready");
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
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          File myFile = SD.open("index1.txt");
          if (myFile) {
            while (myFile.available()) {
              char read = myFile.read();
              client.print((char)read);
            }
            // close the file:
            myFile.close();
            client.println("<br /><div />");
            client.print("ZP01 read value: ");
            client.print(ZP01_value);
            client.println("</div /><br />");
            client.println("  </body>\n</html>");
//            myFile = SD.open("index2.txt");
//            if (myFile) {
//              while (myFile.available()) {
//              char read = myFile.read();
//              Serial.write(read);
//              client.print((char)read);
//            }
//              // close the file:
//              myFile.close();  
//            }
          } else {
            // if the file didn't open, print an error:
            Serial.println("Index1 error");
          }
//          client.println("<html>");
//          client.print("ZPH01 read value: ");
//          client.print(ZPH01_value);
//          client.println("<br />");
//          client.print("ZP01 read value: ");
//          client.print(ZP01_value);
//          client.println("<br />");
//          client.print("SHTC3 read temperature raw: ");
//          client.print(SHTC3_temp_raw);
//          client.println("<br />");
//          client.print("SHTC3 read temperature converted: ");
//          client.print(SHTC3_temp_value/FIXED_POINT_ACCURACY);
//          client.print(".");
//          client.print(SHTC3_temp_value%FIXED_POINT_ACCURACY);
//          client.println("<br/>");
//          client.print("SHTC3 read humidity: ");
//          client.print(SHTC3_hum_value);
//          client.println("<br />");
//          client.println("</html>");
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
