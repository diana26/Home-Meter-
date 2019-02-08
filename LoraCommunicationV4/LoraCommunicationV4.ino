#include <MemoryFree.h>
#include <EEPROM.h>
#include "EmonLib.h"
#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>
#include <string.h>
#include <stdio.h>

//Radio Setup 
#define RF69_FREQ 434.0 // Change to 434.0 or 915.0 depending on the radio.

#define DEST_ADDRESS   6 // Where to send packets to! (P1-Int Dest Address)

#define MY_ADDRESS     3 // Different for every node

#if defined (__AVR_ATmega328P__)  
  #define RFM69_INT     3  // 
  #define RFM69_CS      4  //
  #define RFM69_RST     2  // "A"
  #define LED           13
#endif

// Variables 

 int RORWN;
 int ID = 100;
 int x = 0;
 int count = 0;
 int number = 0;
 int CheckSum = 0;
 int ledPin = 7;
 int ledPin2 = 6;
 int ledPin3 = 5;

 unsigned int startTime;
 unsigned int currentTime;
 unsigned int periodOfTime = 60000;

 // Variables for the Package
 String RORWS = "1AB";
 String PACK;
 String RORW;
 char Div = '$';
 char Coma = ',';

 // Variables for the measurements
 double Vrms = 130; //Value of voltage
 double kw; // declaration of kilowatts in Line1  
 double kw2; // declaration of kilowatts in Line2 
 double kw3;
 double Kwh = 0.0000; // declaration of Kilowatts per hour in both lines 

// Creation of an union for save the Kwh in EEPROM memory
 union {
    byte b[4];
    double d = 0.0000;
  }dato;

 // Objects for the current measurement
 EnergyMonitor emon1;
 EnergyMonitor emon2;
 EnergyMonitor emon3;
// Radio Initialization

RH_RF69 rf69(RFM69_CS, RFM69_INT); // Singleton instance of the radio driver

RHReliableDatagram rf69_manager(rf69, MY_ADDRESS); // Class to manage message delivery and receipt, using the driver declared above

int16_t packetnum = 0;  // packet counter, we increment per xmission

void setup() 
{
  startTime = millis();
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin2, OUTPUT);
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
  
  // Check if the radio is properly connected
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
  uint8_t key[] = "ABCDEFGHIJKLMNOP"; //16 characteres
  rf69.setEncryptionKey(key);
  
  pinMode(LED, OUTPUT);
  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
  
  // Current: input pin, calibration for all the lines

    emon1.current(0, 135.1);
    emon2.current(1, 135.1);
    emon3.current(2, 135.1);
  Serial.println("Irms      Watts      Kw      Kwh      Analog");
  
  // Delete the Kwh information from the EEPROM, use it if you want to reset an Arduino
/*  for (int i = 0; i <4; i++) {
      EEPROM.write(i, 0);
    } */
  
  // Print the value of Kwh from the EEPROM ,and assig the value of the union
  //comment it the first time you run an Arduino, otherwise the Serial prints a "nan"
  for (int i = 0; i < 4; i++) {
    dato.b[i] = EEPROM.read(i);
  }
   Serial.print("killowatts setup: ");
   Serial.println(dato.d);
   Kwh = dato.d;
   Serial.println(millis());
}

// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN]; 
uint8_t data[] = "  OK";


void loop() {
  // Variables for the currents, must not be declared in setup
  currentTime = millis();
  double Irms;
  double Irms2;
  double Irms3;
  
  // Printing the value of Analog Reads in both lines, the pin is the parameter
  int serial = emon1.printingSerial(0);
  int serial2 = emon2.printingSerial(1);
  int serial3 = emon3.printingSerial(2);
  
  // Assign the value of the currents, using calcIrms function from library
  Irms = (emon1.calcIrms(1480));
  Irms2 = (emon2.calcIrms(1480));
  //Irms3 = (emon3.calcIrms(1480));
  Irms3 = 0.0;
  
  //Assign the value of watts in the lines, also wattsTotal
  double watts = (Irms * Vrms);
  double watts2 = (Irms2 * Vrms);
  double watts3 = (Irms3 * Vrms);
  double wattsTotal = watts + watts2 + watts3;
  double KwhAux;
  
  // Use this equation to determine the Kwh value
  Kwh = Kwh + (wattsTotal * (0.5125/60/60));
  
  // Assign the value of kilowatts in the lines (divide watts by 1000)
  kw = (watts/1000); 
  kw2 = (watts2/1000);
  kw3 = (watts3/1000);
  KwhAux = Kwh/1000;
  
  //Printing values for Testing purpose
  Serial.print(Irms); 
  Serial.print("      ");
  Serial.print(Irms2); 
  Serial.print("      ");
  Serial.println(Irms3);

  // Assign the Kwh value to the union, and save it into the EEPROM
  dato.d = Kwh;
  for (int i = 0; i < 4; i++) 
    EEPROM.write(i, dato.b[i]);

  // Create the String package message
  PACK= RORWS+Div+ID+Div+DEST_ADDRESS+Div+Irms+Coma+Irms2+Coma+Irms3+Coma+Vrms+Coma+wattsTotal+Coma+Kwh+Div+MY_ADDRESS+Div;
  // CheckSum to add at the end of the package
  int CheckSum = PACK.length();
  //int CheckSum = 30;
  RORW = PACK+(String)CheckSum;
  char radiopacket[RORW.length()+1];
  RORW.toCharArray(radiopacket,RORW.length()+1);
  
  // Send a message to the DESTINATION!
  // Check approx every minute and send the package to the receiver,
  // When currentTime - startTime is equal or greater than periodOfTime(1 minute)
  // we send the packet
  
  Serial.println(currentTime-startTime);
  if ( currentTime - startTime >= periodOfTime) {
    x = 0;
    ID++;
    if (ID == 1000)
      ID = 100;
      
    // This loop help us to send the same packet again (10 times as max)
    // it breaks when the radio receives an ACK 
    for (int i = 0; i < 10; i++) {
      Serial.println("entrando a sending");
      Serial.print("Sending "); Serial.println(radiopacket);
      if (rf69_manager.sendtoWait((uint8_t *)radiopacket, strlen(radiopacket), DEST_ADDRESS)) {
        Serial.println("entra here");       
        // Now wait for a reply from the server
        uint8_t len = sizeof(buf);
        uint8_t from;   
        if (rf69_manager.recvfromAckTimeout(buf, &len, 2000, &from)) {
          digitalWrite(ledPin2, HIGH);
          buf[len] = 0; // zero out remaining string
          Serial.print("Got reply from #"); Serial.print(from);
          Serial.print(" [RSSI :");
          Serial.print(rf69.lastRssi());
          Serial.print("] : ");
          Serial.println((char*)buf);
          delay(500);
          digitalWrite(ledPin2, LOW);
          break;     
          } else {
          Serial.println("No reply, is anyone listening?");
        }
      } else {
        digitalWrite(ledPin, HIGH);
        Serial.println("Sending failed (no ack)");
        delay(500);
        digitalWrite(ledPin, LOW);
      }
    }
    startTime = currentTime;
  }
  }
