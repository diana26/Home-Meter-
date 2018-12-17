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
 char Div = '$';
 int RORWN;
 double Vrms = sqrt(2) * (125); //Volts
 double kw; //Potencia  
 //double kwh;
 int Ran = random(0 , 9);
 float t;
 int x = 1;
 double Kwh = 0;
 unsigned long endMillis;
 unsigned long startMillis; 
 EnergyMonitor emon1;



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

  emon1.current(1, 111.1);             // Current: input pin, calibration.

}

// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
uint8_t data[] = "  OK";

//--------------------------------------------------------------------------------------/
void loop() {
    
  endMillis = millis();
  unsigned long times = endMillis - startMillis;
  double Irms = (emon1.calcIrms(1480));  // Calculate Irms only 
  double I = Irms / sqrt(2);
  Kwh = Kwh + ((double)I * ((double)times/60/60/1000));
  
  startMillis = millis();
  delay(500);  // Wait 2 seconds between transmits, could also 'sleep' here!

  kw = I*Vrms/1000; //kilowatts
  //kwh = kw*x*t; //kilowatts por hora
  Serial.println("printing values: ");
  //Serial.println(t);
  Serial.print("print kw: ");
  Serial.println(kw);
  Serial.print("print Irms1: ");
  Serial.println(Irms); 
  Serial.print("print I: ");
  Serial.println(I);
  Serial.print("print Vrms");
  Serial.println(Vrms);
  Serial.print("print Kwh");
  Serial.println(Kwh);
  Serial.print("print Analog: ");
  Serial.println(analogRead(1));
  RORW=RORWS+Div+MY_ADDRESS+Div+DEST_ADDRESS+Div+Ran+Div+I+Div+Vrms+Div+kw+Div+Div+Div+Fin;

  
  char radiopacket[RORW.length()+1];
  RORW.toCharArray(radiopacket,RORW.length()+1);
  
  itoa(packetnum++, radiopacket+60, 10);
  Serial.print("Sending "); Serial.println(radiopacket);
  
  // Send a message to the DESTINATION!
  if (rf69_manager.sendtoWait((uint8_t *)radiopacket, strlen(radiopacket), DEST_ADDRESS)) {
    // Now wait for a reply from the server
    uint8_t len = sizeof(buf);
    uint8_t from;   
    if (rf69_manager.recvfromAckTimeout(buf, &len, 2000, &from)) {
      buf[len] = 0; // zero out remaining string
      
      Serial.print("Got reply from #"); Serial.print(from);
      Serial.print(" [RSSI :");
      Serial.print(rf69.lastRssi());
      Serial.print("] : ");
      Serial.println((char*)buf);     
      } else {
      Serial.println("No reply, is anyone listening?");
    }
  } else {
    Serial.println("Sending failed (no ack)");
  }
  x = x+1;
  Serial.println(x);
  }
