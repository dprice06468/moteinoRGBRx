// change color of common anode RGB LED based on serial input
// $R - fade to red
// $G - fade to green
// $B - fade to blue
// $W - fade to white
// $K - fade to black
//
// connect an RGB LED with common anode to the following PWM pins
// set FADESPEED to a higher value to slow the fade

#include <RFM69.h>    //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>//get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>      //comes with Arduino IDE (www.arduino.cc)
#include <SPIFlash.h> //get it here: https://www.github.com/lowpowerlab/spiflash

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define NODEID        1    //unique for each node on same network
#define NETWORKID     100  //the same on all nodes that talk to each other
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
//*********************************************************************************************

#define SERIAL_BAUD   115200

#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for 4mbit  Windbond chip (W25X40CL)
bool promiscuousMode = false; //set to 'true' to sniff all packets on the same network

// define RGB pinouts
#define REDPINOUT 3
#define GREENPINOUT 5
#define BLUEPINOUT 6
 
// make this higher to slow down the fade
#define FADESPEED 10     

// set the initial color to black
int iRedLevel = 255;
int iGreenLevel = 255;
int iBlueLevel = 255;

//------------------------------------------------------------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(10);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.setHighPower(); //only for RFM69HW!
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  //radio.setFrequency(919000000); //set frequency to some custom frequency
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
    
#ifdef ENABLE_ATC
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)");
#endif

  //setup RGB output pins
  pinMode(REDPINOUT, OUTPUT);
  pinMode(GREENPINOUT, OUTPUT);
  pinMode(BLUEPINOUT, OUTPUT);

  // set the initial color to black
  digitalWrite(REDPINOUT, HIGH);
  digitalWrite(GREENPINOUT, HIGH);
  digitalWrite(BLUEPINOUT, HIGH);
  Serial.println("should be black");
}

byte ackCount=0;
uint32_t packetCount = 0;

//------------------------------------------------------------------
void loop() {
  //process any serial input
  if (Serial.available() > 0)
  {
    char charData[20];  //to hold char data received
    String strData;     //to hold string data received

    strData = Serial.readString();

    Serial.println("==> " + strData);

    strData.toCharArray(charData, 20);
    switch (charData[0]) {
      case '%':
        changeColor(strData);
        break;
      case '$':
        for (byte j = 1; j < strData.length(); j++) {
          Serial.println("processing char " + (String)j + ": " + charData[j]);
          changeColor(charData[j]);
        }
        break;
    }
  }

  if (radio.receiveDone())
  {
    char charData[10];  //to hold char data received
    String strData;     //to hold string data received
    byte i;
    
    Serial.print("#[");
    Serial.print(++packetCount);
    Serial.print(']');
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");

    for (i = 0; i < radio.DATALEN; i++) {
      Serial.print((char)radio.DATA[i]);
      charData[i] = (char)radio.DATA[i];
    }
    charData[i] = '\0';
    strData = String(charData);
    Serial.println(" ==> " + strData);
    
    Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");

    switch (charData[0]) {
      case '%':
        changeColor(strData);
        break;
      case '$':
        for (byte j = 1; j < strData.length(); j++) {
          Serial.println("processing char " + (String)j + ": " + charData[j]);
          changeColor(charData[j]);
        }
        break;
    }
    
    Serial.println();
    Blink(LED,3);
  }
}

//------------------------------------------------------------------
void changeColor(String strColor) {
  byte r = strColor.indexOf('r');
  byte g = strColor.indexOf('g');
  byte b = strColor.indexOf('b');

  String strRVal = strColor.substring(r+1,g);
  String strGVal = strColor.substring(g+1,b);
  String strBVal = strColor.substring(b+1);

  Serial.println("changing color to r:" + strRVal + ",g:" + strGVal + ",b:" + strBVal);
  
  iRedLevel = 255 - strRVal.toInt();
  iGreenLevel = 255 - strGVal.toInt();
  iBlueLevel = 255 - strBVal.toInt();
  analogWrite(REDPINOUT, iRedLevel);
  analogWrite(GREENPINOUT, iGreenLevel);
  analogWrite(BLUEPINOUT, iBlueLevel);
}

//------------------------------------------------------------------
void changeColor(char input) {
  switch (input) {
    case 'k':
      Serial.println("instant black");
      instantBlack();
      Serial.println("should be black");
      break;
    case 'K':
      Serial.println("fading to black");
      fadeToBlack();
      Serial.println("should be black");
      break;
    case 'r':
      Serial.println("instant red");
      instantRed();
      Serial.println("should be red");
      break;
    case 'R':
      Serial.println("fading to red");
      fadeToRed();
      Serial.println("should be red");
      break;
    case 'g':
      Serial.println("instant green");
      instantGreen();
      Serial.println("should be green");
      break;
    case 'G':
      Serial.println("fading to green");
      fadeToGreen();
      Serial.println("should be green");
      break;
    case 'b':
      Serial.println("instant blue");
      instantBlue();
      Serial.println("should be blue");
      break;
    case 'B':
      Serial.println("fading to blue");
      fadeToBlue();
      Serial.println("should be blue");
      break;
    case 'w':
      Serial.println("instant white");
      instantWhite();
      Serial.println("should be white");
      break;
    case 'W':  
      Serial.println("fading to white");
      fadeToWhite();
      Serial.println("should be white");
      break;
    case 't':
      {
        byte temperature =  radio.readTemperature(-1); // -1 = user cal factor, adjust for correct ambient
        byte fTemp = 1.8 * temperature + 32; // 9/5=1.8
        Serial.print( "Radio Temp is ");
        Serial.print(temperature);
        Serial.print("C, ");
        Serial.print(fTemp); //converting to F loses some resolution, obvious when C is on edge between 2 values (ie 26C=78F, 27C=80F)
        Serial.println('F');
        break;
      }
  }
}

//------------------------------------------------------------------
void instantBlack() {
  digitalWrite(REDPINOUT, HIGH);
  digitalWrite(GREENPINOUT, HIGH);
  digitalWrite(BLUEPINOUT, HIGH);
  iRedLevel = 255;
  iGreenLevel = 255;
  iBlueLevel = 255;
}

//------------------------------------------------------------------
void fadeToBlack() {
  while (iRedLevel < 255 | iGreenLevel < 255 | iBlueLevel < 255) {
    iRedLevel = (iRedLevel < 255) ? iRedLevel+1 : 255;
    iGreenLevel = (iGreenLevel < 255) ? iGreenLevel+1 : 255;
    iBlueLevel = (iBlueLevel < 255) ? iBlueLevel+1 : 255;
  
    analogWrite(REDPINOUT, iRedLevel);
    analogWrite(GREENPINOUT, iGreenLevel);
    analogWrite(BLUEPINOUT, iBlueLevel);
    delay(FADESPEED);
  } 
  instantBlack();
}

//------------------------------------------------------------------
void instantRed() {
  digitalWrite(REDPINOUT, LOW);
  digitalWrite(GREENPINOUT, HIGH);
  digitalWrite(BLUEPINOUT, HIGH);
  iRedLevel = 0;
  iGreenLevel = 255;
  iBlueLevel = 255;
}

//------------------------------------------------------------------
void fadeToRed() {
  while (iRedLevel > 0 | iGreenLevel < 255 | iBlueLevel < 255) {
    Serial.println(iRedLevel);
    iRedLevel = (iRedLevel > 0) ? iRedLevel-1 : 0;
    iGreenLevel = (iGreenLevel < 255) ? iGreenLevel+1 : 255;
    iBlueLevel = (iBlueLevel < 255) ? iBlueLevel+1 : 255;
  
    analogWrite(REDPINOUT, iRedLevel);
    analogWrite(GREENPINOUT, iGreenLevel);
    analogWrite(BLUEPINOUT, iBlueLevel);
    delay(FADESPEED);
  } 
  instantRed();
}

//------------------------------------------------------------------
void instantGreen() {
  digitalWrite(REDPINOUT, HIGH);
  digitalWrite(GREENPINOUT, LOW);
  digitalWrite(BLUEPINOUT, HIGH);
  iRedLevel = 255;
  iGreenLevel = 0;
  iBlueLevel = 255;
}

//------------------------------------------------------------------
void fadeToGreen() {
  while (iRedLevel < 255 | iGreenLevel > 0 | iBlueLevel < 255) {
    iRedLevel = (iRedLevel < 255) ? iRedLevel+1 : 255;
    iGreenLevel = (iGreenLevel > 0) ? iGreenLevel-1 : 0;
    iBlueLevel = (iBlueLevel < 255) ? iBlueLevel+1 : 255;
  
    analogWrite(REDPINOUT, iRedLevel);
    analogWrite(GREENPINOUT, iGreenLevel);
    analogWrite(BLUEPINOUT, iBlueLevel);
    delay(FADESPEED);
  } 
  instantGreen();
}

//------------------------------------------------------------------
void instantBlue() {
  digitalWrite(REDPINOUT, HIGH);
  digitalWrite(GREENPINOUT, HIGH);
  digitalWrite(BLUEPINOUT, LOW);
  iRedLevel = 255;
  iGreenLevel = 255;
  iBlueLevel = 0;
}

//------------------------------------------------------------------
void fadeToBlue() {
  while (iRedLevel < 255 | iGreenLevel < 255 | iBlueLevel > 0) {
    iRedLevel = (iRedLevel < 255) ? iRedLevel+1 : 255;
    iGreenLevel = (iGreenLevel < 255) ? iGreenLevel+1 : 255;
    iBlueLevel = (iBlueLevel > 0) ? iBlueLevel-1 : 0;
  
    analogWrite(REDPINOUT, iRedLevel);
    analogWrite(GREENPINOUT, iGreenLevel);
    analogWrite(BLUEPINOUT, iBlueLevel);
    delay(FADESPEED);
  } 
  instantBlue();
}

//------------------------------------------------------------------
void instantWhite() {
  digitalWrite(REDPINOUT, LOW);
  digitalWrite(GREENPINOUT, LOW);
  digitalWrite(BLUEPINOUT, LOW);
  iRedLevel = 0;
  iGreenLevel = 0;
  iBlueLevel = 0;
}

//------------------------------------------------------------------
void fadeToWhite() {
  while (iRedLevel > 0 | iGreenLevel > 0 | iBlueLevel > 0) {
    iRedLevel = (iRedLevel > 0) ? iRedLevel-1 : 0;
    iGreenLevel = (iGreenLevel > 0) ? iGreenLevel-1 : 0;
    iBlueLevel = (iBlueLevel > 0) ? iBlueLevel-1 : 0;
  
    analogWrite(REDPINOUT, iRedLevel);
    analogWrite(GREENPINOUT, iGreenLevel);
    analogWrite(BLUEPINOUT, iBlueLevel);
    delay(FADESPEED);
  } 
  //instantWhite();
}

//------------------------------------------------------------------
void Blink(byte PIN, int DELAY_MS) {
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}
