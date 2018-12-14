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

 String RORWS="Green";
 String RORW;
 String Fin = "END";
 String Div = "$";
 int RORWN;
 double Vrms = sqrt(2) * (125); //Volts
 double watts; //Potencia  
 //double kwh;
 int Ran = random(0 , 9);
 float t;
 int x = 1;
 double Kwh;
 //double KwhAux;
 unsigned long endMillis;
 unsigned long startMillis;
 int count = 0;
 //double coeff = 3.3/1024.0; 
 EnergyMonitor emon1;

union double_Khw {
  double AuxKwh;
  byte AuxKwhByte[4];
} unionKwh;


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

  emon1.current(0, 111.1);             // Current: input pin, calibration.
  Serial.print("Reading memory: ");
  //making the new killowatts hour variable
  /*for (int i = 0; i < 4; i++) {
    EEPROM.write(i, 0);
  }*/
  Serial.println(unionKwh.AuxKwh);
  unionKwh.AuxKwhByte[0] = EEPROM.read(0);
  unionKwh.AuxKwhByte[1] = EEPROM.read(1);
  unionKwh.AuxKwhByte[2] = EEPROM.read(2);
  unionKwh.AuxKwhByte[3] = EEPROM.read(3);
  Serial.println(unionKwh.AuxKwh);
  Kwh = unionKwh.AuxKwh;
}

// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
uint8_t data[] = "  OK";

//--------------------------------------------------------------------------------------/
void loop() {
    
  endMillis = millis();
  double I, Irms;
  unsigned long times = endMillis - startMillis;
  
  Irms = (emon1.calcIrms(1480));  // Calculate Irms only 
  I = Irms / sqrt(2);
  watts = I*Vrms;
  unionKwh.AuxKwh = Kwh;
    Kwh = Kwh + ((double)watts * ((double)times/60/60/1000));
    EEPROM.write(0, unionKwh.AuxKwhByte[0]);
    EEPROM.write(1, unionKwh.AuxKwhByte[1]);
    EEPROM.write(2, unionKwh.AuxKwhByte[2]);
    EEPROM.write(3, unionKwh.AuxKwhByte[3]);
    startMillis = millis();
    //delay(1000);  
    Serial.println(Irms);
    Serial.println(I);
    //Serial.println(Kwh);
    int serial = emon1.printingSerial(0);
    Serial.println(serial);
    //Serial.println((analogRead(0)-514));
    RORW=RORWS+Div+MY_ADDRESS+Div+DEST_ADDRESS+Div+Ran+Div+I+Div+Vrms+Div+watts+Div+Kwh+Div+Fin;
  
    
    char radiopacket[RORW.length()+1];
    RORW.toCharArray(radiopacket,RORW.length()+1);
    
    itoa(packetnum++, radiopacket+60, 10);
   if (rf69_manager.sendtoWait((uint8_t *)radiopacket, strlen(radiopacket), DEST_ADDRESS)) {
      // Now wait for a reply from the server
      uint8_t len = sizeof(buf);
      uint8_t from;   
      if (rf69_manager.recvfromAckTimeout(buf, &len, 2000, &from)) {
        buf[len] = 0; // zero out remaining string
        
//        Serial.print("Got reply from #"); Serial.print(from);
//        Serial.print(" [RSSI :");
//        Serial.print(rf69.lastRssi());
//        Serial.print("] : ");
//        Serial.println((char*)buf);     
        } else {
        //Serial.println("No reply, is anyone listening?");
      }
    } else {
      //Serial.println("Sending failed (no ack)");
    } 
}
