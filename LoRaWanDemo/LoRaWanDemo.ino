/* @brief  Demo example for using an Arduino compatible LoRaWAN board with
 * some peripherals (BME280 sensor and LCD screen). 
 * Heltec Wifi LoRa 32 v2 board.
 *
 * Description:
 * 1. Initializes board and peripherals while displaying it on the screen.
 * 2. Joins the LoRa gateway.
 * 3. Runs a measurement and send the values with a LoRa packet.
 * 4. Goes to sleep with a random waking up time.
 * 5. Loop.
 *  
 * 
 * @author: F. Moreno Cruz (f.morenocruz@kiotera.de)
 * @company: Kiotera GmbH
 *
 * Based on: Heltec Automation LoRaWAN communication example.
 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
 * www.heltec.org
 */



/* For LoRaWan func */
#include "LoRaWan_APP.h"

/* For display func */
#include <Wire.h>               
#include "HT_SSD1306Wire.h"
#include "images.h"

#include "esp32/rom/rtc.h"

// Sensor BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Parameter for BME280 sensor
#define SEALEVELPRESSURE_HPA (1028)

// Option of automatic DevEui by hardware
#define LORAWAN_DEVEUI_AUTO 0


/* BME280 sensor I2C configuration */
Adafruit_BME280 bme; 

/* variables for BME280 sensor */
float temp; // temperature
float pres; // pressure
float humi; // humidity


/* Configuration of display (included on board) */
SSD1306Wire  boardDisplay(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst
// Aux String for printing in display
char str[30];


/**
 ---- LoRaWAN CONFIGURATION ----------------
*/

/* LoRaWan: OTAA para*/
uint8_t devEui[] = { 0x22, 0x32, 0x33, 0x00, 0x00, 0x88, 0x88, 0x02 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88 };

/* LoRaWan: ABP para*/
uint8_t nwkSKey[] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda,0x85 };
uint8_t appSKey[] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef,0x67 };
uint32_t devAddr =  ( uint32_t )0x007e6ae1;

/*LoraWan: channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan: region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan: Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*LoRaWan: application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

/*LoRaWan: OTAA or ABP*/
bool overTheAirActivation = true;

/*LoRaWan: ADR enable*/
bool loraWanAdr = true;

/*LoRaWan: Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/*LoRaWan: Application port */
uint8_t appPort = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;


/**
 ---- LoRaWAN CONFIGURATION  - END -  ----------------
*/



/* Sleep management  */
RTC_DATA_ATTR bool firstRun = true;



/* Screen config */
void VextON(void){
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}




void setup() {
// Sensor BME280 status
  unsigned status;

/* System Init  ----------------- */ 
  Serial.begin(115200); 
  Mcu.begin();

  if(firstRun){
    firstRun = false;
  }



/* Init LoRaWan -----------------*/

  deviceState = DEVICE_STATE_INIT;



/* Init display ----------------- */

  VextON();
  delay(100);

  // Initialising the UI will init the display too.
  boardDisplay.init();

  if(rtc_get_reset_reason(0) == DEEPSLEEP_RESET){
    // 
    drawText("Deep sleep RST");
  }else{
    // clear the display
    boardDisplay.clear();
    // Display KIOTERA img
    drawKioteraImage();
  }


/* Init sensor BME280 ----------------- */

    // address = 76h but I2C used, has to be changed
    
    // changing by-default I2C pins
    Wire1.setPins(T4,SCL);
    status = bme.begin(0x76, &Wire1);  

    delay(100);

    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    }




} // end setup




/** Draw init bar in included screen
*/
void drawInitProgressBar() {

  boardDisplay.setFont(ArialMT_Plain_10);

  for (int progress=0; progress<=100; progress++){
    
    // clear the display
    boardDisplay.clear();

    // draw the progress bar
    boardDisplay.drawProgressBar(0, 50, 120, 10, progress);

    // draw the percentage as String
    boardDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
    boardDisplay.drawString(64, 33, String(progress) + "%");


      // draw the percentage as String
    boardDisplay.setTextAlignment(TEXT_ALIGN_CENTER);
    boardDisplay.drawString(boardDisplay.width()/2, 5, "Initializing LoRaWan node");
  
    // write the buffer to the display
    boardDisplay.display();
    delay(10);
  }

  delay(500);
  boardDisplay.clear();
  boardDisplay.display();

}

/** Draw Kiotera image in included screen
*/
void drawKioteraImage() {

  boardDisplay.clear();

  // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
  // on how to create xbm files
  boardDisplay.drawXbm(0, 20, kioteraLogo_width, kioteraLogo_height, kioteraLogo_bits);

  // write the buffer to the display
  boardDisplay.display();
  delay(1000);

}

/** Draw text in included screen
*/
void drawText(char str[30]){

  int x = 0;
  int y = 0;

  boardDisplay.clear();

  boardDisplay.setFont(ArialMT_Plain_16);
  boardDisplay.setTextAlignment(TEXT_ALIGN_CENTER); 
  // The coordinates define the center of the text
  x = boardDisplay.width()/2;
  y = boardDisplay.height()/2-7;

  boardDisplay.drawString(x, y, str);

  // write the buffer to the display
  boardDisplay.display();
  delay(1000);
  boardDisplay.clear();
  boardDisplay.display();

}



/** Trigger and print measurement of sensor BME280
*/
void BME280measure() {

  temp=bme.readTemperature();
  pres=bme.readPressure() / 100.0F;
  humi=bme.readHumidity();

  Serial.print("Temperature = ");
  Serial.print(temp);
  Serial.println(" °C.");

  Serial.print("Pressure = ");
  Serial.print(pres);
  Serial.print(" hPa. Which means an approx. altitude of ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m.");

  Serial.print("Humidity = ");
  Serial.print(humi);
  Serial.println(" %.");

  Serial.println();
}



/** LoRaWan: Prepare the payload of the frame
 */
static void prepareTxFrame( uint8_t port )
{
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */

  int pos=int(temp);
  int dec= int((temp - pos) * 100);

  appData[0] = pos;
  appData[1] = dec;

  pos=int(pres);
  dec= int((pres - pos) * 100);

  appData[2] = int(pos/100);
  appData[3] = pos - appData[2]*100;
  appData[4] = dec;

  pos=int(humi);
  dec= int((humi-pos) * 100);

  appData[5] = pos;
  appData[6] = dec;

  appDataSize = 7;

}


/** Aux for printing uint8_t arrays in Hexadecimal format
*/
void printHex(uint8_t num) {
  char hexCar[2];

  sprintf(hexCar, "%02X", num);
  Serial.print(hexCar);
}





void loop()
{

  // clear the display
  boardDisplay.clear();


  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
      // Display init
      drawInitProgressBar();

#if(LORAWAN_DEVEUI_AUTO)
      // Display generating DevEUi by chip ID
      drawText("Generating DevEui");
      Serial.print("Generated DevEui:  ");
      LoRaWAN.generateDeveuiByChipID();
      for(int i=0; i<sizeof(devEui); i++){
        printHex(devEui[i]);
      }
      Serial.println();
#endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
      break;
    }


    case DEVICE_STATE_JOIN:
    {
      // Display joining
      drawText("Joining net");
      LoRaWAN.join();
      break;
    }


    case DEVICE_STATE_SEND:
    {
      // Display sending
      drawText("Sending Lora msg");

      BME280measure();
      prepareTxFrame( appPort );
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }


    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      // Display next packet time
      sprintf(str,"Next msg in %d s",txDutyCycleTime/1000);
      drawText(str);
      deviceState = DEVICE_STATE_SLEEP;
      drawText("Going to sleep");
      break;
    }


    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep(loraWanClass);
      break;
    }


    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }

}



