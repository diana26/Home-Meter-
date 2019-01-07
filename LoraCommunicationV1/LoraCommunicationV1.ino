#include <MemoryFree.h>
#include <EEPROM.h>
#include "EmonLib.h"
#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>
#include <string.h>
#include <stdio.h>

//Radio Setup ------------------------------------------------------------------------/

#define RF69_FREQ 434.0 // Change to 434.0 or 915.0 depending on the radio.

#define DEST_ADDRESS   1 // Where to send packets to! (P1-Int Dest Address)

#define MY_ADDRESS     3 // Different for every node

#if defined (__AVR_ATmega328P__)  
  #define RFM69_INT     3  // 
  #define RFM69_CS      4  //
  #define RFM69_RST     2  // "A"
  #define LED           13
#endif

// Variables -------------------------------------------------------------------------/

 int RORWN;
 int ID = 100;
 int x = 0;
 int count = 0;
 int number = 0;
 int CheckSum = 0;
 
 String RORWS="001";
 String PACK;
 String RORW;
 
 char Div = '$';
 char Coma = ',';
 
 //double Vrms = sqrt(2) * (125); //Volts
 double Vrms = 125;
 double kw; //Potencia 
 double kw2;
 double Kwh = 0.0000;
 double Kwh2 = 0.0000; 
 //double kwh;
 
 float t;
 
 unsigned long endMillis;
 unsigned long startMillis; 

 union {
    byte b[4];
    double d = 0.0000;
  }dato;

  

 EnergyMonitor emon1;
 EnergyMonitor emon2;

// Radio Initialization---------------------------------------------------------------/

RH_RF69 rf69(RFM69_CS, RFM69_INT); // Singleton instance of the radio driver

RHReliableDatagram rf69_manager(rf69, MY_ADDRESS); // Class to manage message delivery and receipt, using the driver declared above

int16_t packetnum = 0;  // packet counter, we increment per xmission

//-------------------------------------------------------------------------------------/

void setup() 
{
  Serial.begin(9600);
  startMillis = millis();
  int minCurr = 1000;
  //while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  pinMode(LED, OUTPUT);     
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);
  
  Serial.println("Feather Addressed RFM69 TX Test!");
  Serial.println();

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  
  if (!rf69_manager.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = "LORENAHERMOSILLO"; //16 caracteres
  rf69.setEncryptionKey(key);
  
  pinMode(LED, OUTPUT);
  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");

  emon1.current(0, 131.1);             // Current: input pin, calibration.
  emon2.current(1, 131.1);
  Serial.println("Irms      Watts      Kw      Kwh      Analog"); 
//  for (int i = 0; i <4; i++) {
//    EEPROM.write(i, 0);
//  }
//  for (int i = 0; i < 4; i++) {
//    dato.b[i] = EEPROM.read(i);
//  }
//   Serial.print("killowatts setup: ");
//   Serial.println(dato.d);
//   Kwh = dato.d;
//}

// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
uint8_t data[] = "  OK";
}
//--------------------------------------------------------------------------------------/
void loop() {
  endMillis = millis();
  double Irms;
  double Irms2;
  unsigned long times = endMillis - startMillis;
  int serial = emon1.printingSerial(0);
  Irms = (emon1.calcIrms(1480));
  Irms2 = (emon2.calcIrms(1480));
  double I = Irms / sqrt(2);
  double watts = (Irms * Vrms);
  double watts2 = (Irms2 * Vrms);
  double wattsTotal = watts + watts2;
  //Kwh = Kwh + ((double)watts * ((double)times/60/60/60/100000.0));
  startMillis = millis();
  Kwh = Kwh + (wattsTotal * (0.5125/60/60));
  //Kwh2 = Khw2 + (watts2 * (0.5125/60/60));
  //delay(500);  // Wait 2 seconds between transmits, could also 'sleep' here!
  kw = (watts/1000); //kilowatts
  kw2 = (watts2/1000);
  //Serial.println(t);
  //Serial.print("print kw: ");
  Serial.print(Irms); 
  Serial.print("      ");
  Serial.print(watts);
  Serial.print("      ");
  Serial.print(kw);
  Serial.print("      ");
  
  Serial.print(Kwh, 4);
  Serial.print("      ");
  Serial.print(Irms2); 
  Serial.print("      ");
  Serial.print(watts2);
  Serial.print("      ");
  Serial.println(kw2);
  Serial.print("      ");
  //Serial.print(Kwh, 4);
//  Serial.print("      ");
  //Serial.println(serial);
  dato.d = Kwh;
  for (int i = 0; i < 4; i++) {
    EEPROM.write(i, dato.b[i]);
  }
  

  PACK=RORWS+Div+ID+Div+DEST_ADDRESS+Div+Irms+Coma+Irms2+Coma+Vrms+Coma+wattsTotal+Coma+Kwh+Div+MY_ADDRESS+Div;
  //PACK=RORWS+Div+ID;
  //CheckSum --------------------------------------------------------------------------/
  int CheckSum = PACK.length();
  //-----------------------------------------------------------------------------------/
  
  RORW = PACK+(String)CheckSum;
  
  char radiopacket[RORW.length()+1];
  RORW.toCharArray(radiopacket,RORW.length()+1);
  
  itoa(packetnum++, radiopacket+60, 10);
  //Serial.print("Sending "); Serial.println(radiopacket);
  
  // Send a message to the DESTINATION!
  
  if ( x == 32) {
    Serial.println("entrando a sending");
    x = 0;
    ID++;
    if (rf69_manager.sendtoWait((uint8_t *)radiopacket, strlen(radiopacket), DEST_ADDRESS)) {
      Serial.println("entra here");
      // Now wait for a reply from the server
      uint8_t len = sizeof(buf);
      uint8_t from;   
      if (rf69_manager.recvfromAckTimeout(buf, &len, 2000, &from)) {
        buf[len] = 0; // zero out remaining string
        
        Serial.print("Got reply from #"); Serial.print(from);
        Serial.print(" [RSSI :");
        Serial.print(rf69.lastRssi());
        Serial.print("] : ");
        Serial.print(" [RSSI2 :");
        Serial.print(rf69.rssiRead());
        Serial.print("] : ");
        Serial.println((char*)buf);     
        } else {
        Serial.println("No reply, is anyone listening?");
      }
    } else {
      Serial.println("Sending failed (no ack)");
      Serial.println(rf69.rssiRead());
    }
    }
    //delay(20000);
    else if ( x != 32) {
      x++;
    }
  //Serial.println(x);
  //count++;
  }
