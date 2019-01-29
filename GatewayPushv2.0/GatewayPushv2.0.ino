#include <MemoryFree.h>
#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
const char website[] PROGMEM = "radika.io";
static byte session;

byte Ethernet::buffer[500];
Stash stash;
// Example 2 - Receive with an end-marker

// Variables for the packet to push
const byte numChars = 65;
char receivedChars[numChars];   // an array to store the received data
int eraseAux;
String voltageL,ampsL1,ampsL2,ampsL3,kwL1,kwhL1;
String meterNumber;
String medidorName;
boolean newData = false;
double voltajeL = 0;
double corrienteL1 = 0;
double corrienteL2 = 0;
double corrienteL3 = 0;
double kilowattL1 = 0;
double consumoL1 = 0;
int count = 0;
int CodeNumber;

//Leds indicators
int ledPin2 = 6; 
int ledPin3 = 7;

static void sendPost () {
  Serial.println(F("Sending Post request..."));
  byte sd = stash.create();
  stash.print("{\"meter\":\"");
  stash.print(medidorName);
  stash.print("\",");
  stash.print("\"data\":[{");
  stash.print("\"l1\":{\"v");
  stash.print("\":");
  stash.print("\"");
  stash.print(voltajeL);
  stash.print("\",");
  stash.print("\"a\":\"");
  stash.print(corrienteL1);
  stash.print("\",");
  stash.print("\"kw\":\"");
  stash.print(voltajeL*corrienteL1);
  stash.print("\",");
  stash.print("\"kwh\":\"");
  stash.print(consumoL1);
  stash.print("\"}");
  stash.print(",\"l2\":{\"v");
  stash.print("\":\"");
  stash.print(voltajeL);
  stash.print("\",\"a\":\"");
  stash.print(corrienteL2);
  stash.print("\",\"kw\":\"");
  stash.print(voltajeL*corrienteL2);
  stash.print("\",\"kwh\":\"");
  stash.print(consumoL1);
  stash.print("\"},");
  stash.print("\"l3\":{\"v");
  stash.print("\":\"");
  stash.print(voltajeL);
  stash.print("\",\"a\":\"");
  stash.print(corrienteL3);
  stash.print("\",\"kw\":\"");
  stash.print(voltajeL*corrienteL3);
  stash.print("\",\"kwh\":\"");
  stash.print(consumoL1);
  stash.print("\"},");
  stash.print("\"kwt\":\"");
  stash.print(kilowattL1);
  stash.print("\"");
  stash.print("}]");
  stash.print(",\"status\":{\"code\":\"");
  stash.print(CodeNumber);
  stash.print("\"}}");
  stash.save();
  int stash_size = stash.size();
  //Serial.println(stash_size);
  // Compose the http POST request, taking the headers below and appending
  // previously created stash in the sd holder.
  Stash::prepare(PSTR("POST http://$F/api.php HTTP/1.0" "\r\n"
    "Host: $F" "\r\n"
    "Content-Length: $D" "\r\n"
    "Content-Type: application/json \r\n"
    "\r\n"
    "$H"),
   website,website,stash_size, sd);

  // send the packet - this also releases all stash buffers once done
  // Save the session ID so we can watch for it in the main loop.
  session = ether.tcpSend();
}

void setup() {
    Serial.begin(9600);
    pinMode(ledPin2, OUTPUT);
    pinMode(ledPin3, OUTPUT);
    Serial.println("<Arduino is ready>");

    // Change 'SS' to your Slave Select pin, if you arn't using the default pin
//    if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0)
//      Serial.println(F("Failed to access Ethernet controller"));
//    if (!ether.dhcpSetup())
//      Serial.println(F("DHCP failed"));
    connectToInternet();
    ether.printIp("IP:  ", ether.myip);
    ether.printIp("GW:  ", ether.gwip);
    ether.printIp("DNS: ", ether.dnsip);
    
    //ether.printIp("SRV: ", ether.hisip);
    
    ether.printIp("SRV: ", ether.hisip);
}

void loop() {
    //Serial.print(F("freeMemory loop = "));
    //Serial.println(freeMemory());
    recvWithEndMarker();
    ether.packetLoop(ether.packetReceive());
    //getResponseInternet();
//
    const char* reply = ether.tcpReply(session);
    if (reply != 0) {
      String ans = String(reply);
      //Serial.println(ans);
      //String aux = "";
      int p1 = 0, p2 = 1, x = ans.length();
      //Serial.println(x);
      while (x > 0) {
        if ( ans[p1] == '2') {
          //Serial.println("enter here");
          //aux += ans[p1];
          if (ans[p2] == '0' && ans[p2 + 1] == '0') {
              digitalWrite(ledPin2, HIGH);
              delay(1000);
              digitalWrite(ledPin2, LOW);
              break;
          }       
        }
        else
          p1++, p2++;
        x--;
      }
      Serial.println(F("Got a response!"));
      Serial.println(reply);
      count  = 0;
    }
//    else {
//      Serial.print(F("no response"));
//      count++;
//      Serial.println(count);
//      if (count == 2) {
//      digitalWrite(ledPin3, HIGH);  
//      while (!connectToInternet) {
//        connectToInternet();
//        delay(2000);
//        }
//      }
//    }
}

void recvWithEndMarker() {
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
    String measureData;
    
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();
        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            //Serial.println(measureData);
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
            
            //Serial.println(F("La informacion es: "));
            measureData = String(receivedChars);
            Serial.println(measureData);
            segmentData(measureData);
            receiveNewData();
            //Serial.println(medidorName);
            sendPost();
           // ether.packetLoop(ether.packetReceive());
            //getResponseInternet();
            
            voltageL = "";
            ampsL1 = "";
            ampsL2 = "";
            ampsL3 = "";
            kwL1 = "";
            kwhL1 = "";
            meterNumber = "";
        }
    }
}

bool connectToInternet() {
  if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0) {
      Serial.println(F("Failed to access Ethernet controller"));
  }
  if (!ether.dhcpSetup()) {
      Serial.println(F("DHCP failed"));
      return 0;
  }
  ether.parseIp(ether.hisip,"132.148.226.34");
  digitalWrite(ledPin3, LOW);
  return 1;
}

void showNewData() {
    if (newData == true) {
        Serial.print("This just in ... ");
        Serial.println(receivedChars);
        newData = false;
    }
}

void segmentData(String aux)
{
  if(aux != "" || aux != "\0")
  {
    //Serial.println(aux);
    getMeterNumber(aux);
    aux.remove(0,eraseAux+1);
    getAmpsL1(aux);
    aux.remove(0,eraseAux+1);
    getAmpsL2(aux);
    aux.remove(0,eraseAux+1);
    getAmpsL3(aux);
    aux.remove(0,eraseAux+1);
    getVoltage(aux);
    aux.remove(0,eraseAux+1);
    getPower(aux);
    aux.remove(0,eraseAux+1);
    getPowerConsumption(aux);
    aux.remove(0,eraseAux+1);
    Serial.println(aux);
    ErrorCode(aux);
  }
}

void getMeterNumber(String aux)
{
  if(aux != "" || aux != "\0")
  {
    int i = 0;
    eraseAux = 0;

    while(aux[i] != ',')
    {
      meterNumber += aux[i];
      i++;
      eraseAux++;
    }
    medidorName = "Green" + meterNumber;
  }
}
void getVoltage(String aux)
{
  //Serial.println(aux);
  if(aux != "" || aux != "\0")
  {
    int i = 0;
    eraseAux = 0;
    while(aux[i] != ',')
    {
      voltageL += aux[i];
      i++;
      eraseAux++;
    }
    //Serial.println(voltageL1);
    voltajeL = voltageL.toDouble();
    //Serial.println(voltajeL);
  }
}

void getAmpsL1(String aux)
{
  //Serial.println(aux);
  if(aux != "" || aux != "\0")
  {
    int i = 0;
    eraseAux = 0;
    while(aux[i] != ',')
    {
      ampsL1 += aux[i];
      i++;
      eraseAux++;
    }
    corrienteL1 = ampsL1.toDouble();
  }
}

void getAmpsL2(String aux)
{
  //Serial.println(aux);
  if(aux != "" || aux != "\0")
  {
    int i = 0;
    eraseAux = 0;
    while(aux[i] != ',')
    {
      ampsL2 += aux[i];
      i++;
      eraseAux++;
    }
    corrienteL2 = ampsL2.toDouble();
  }
}

void getAmpsL3(String aux)
{
  if(aux != "" || aux != "\0")
  {
    int i = 0;
    eraseAux = 0;
    while(aux[i] != ',')
    {
      ampsL3 += aux[i];
      i++;
      eraseAux++;
    }
    corrienteL3 = ampsL3.toDouble();
    //Serial.println(corrienteL3);
  }
}

void getPower(String aux)
{
  //Serial.println(aux);
  if(aux != "" || aux != "\0")
  {
    int i = 0;
    eraseAux = 0;
    while(aux[i] != ',')
    {
      kwL1 += aux[i];
      i++;
      eraseAux++;
    }
    //Serial.println(kwL1);
    kilowattL1 = kwL1.toDouble();
    //Serial.println(kilowattL1);
  }
}

void getPowerConsumption(String aux)
{
  Serial.println(aux);
  if(aux != "" || aux != "\0")
  {
    int i = 0;
    eraseAux = 0;
    while(aux[i] != ',')
    {
      kwhL1 += aux[i];
      i++;
      eraseAux++;
    }
    //Serial.println(kwhL1);
    consumoL1 = kwhL1.toDouble();
    //Serial.println(consumoL1);
  }
}


void receiveNewData()
{
  if(newData == true)
  {
    newData = false;
  }
}

void getResponseInternet()
{
  //ether.packetLoop(ether.packetReceive());

    const char* reply = ether.tcpReply(session);
    if (reply != 0) {
      String ans = String(reply);
      Serial.println(ans);
      //String aux = "";
      int p1 = 0, p2 = 1, x = ans.length();
      Serial.println(x);
      while (x > 0) {
        if ( ans[p1] == '2') {
          Serial.println("enter here");
          //aux += ans[p1];
          if (ans[p2] == '0' && ans[p2 + 1] == '0') {
              digitalWrite(ledPin2, HIGH);
              delay(1000);
              digitalWrite(ledPin2, LOW);
              break;
          }       
        }
        else
          p1++, p2++;
        x--;
      }
      Serial.println(F("Got a response!"));
      Serial.println(reply);
      count  = 0;
    }
    else {
      Serial.print(F("no response"));
      count++;
      Serial.println(count);
      if (count == 2) {
        Serial.println(F("LED PRENDIDO"));
      digitalWrite(ledPin3, HIGH);
      delay(2000);
      digitalWrite(ledPin3, LOW);  
      while (!connectToInternet) {
        connectToInternet();
        delay(2000);
        }
      }
    }
}

void ErrorCode (String ErrorAux) {
    if(ErrorAux != "" || ErrorAux != "\0") {
      Serial.println(ErrorAux);
      if (ErrorAux[0] == 'O') {
        Serial.println(F("Todo correcto"));
        CodeNumber = 200;
      }
      else if(ErrorAux[0] == 'E'){
        switch (ErrorAux[1])
        {
          case '1':
            CodeNumber = 900;
            break;
            
          case '2':
            CodeNumber = 901;
            break;
            
          case '3':
            CodeNumber = 902;
            break;
            
          case '4':
            CodeNumber = 903;
            break;
            
          case '5':
            CodeNumber = 904;
            break;
            
          default: 
            CodeNumber = 0;
            break;
        }
      }
    }
    else
      return;
}
