/* Heltec Automation LoRaWAN communication example
 *
 * Function:
 * 1. Upload node data to the server using the standard LoRaWAN protocol.
 *  
 * Description:
 * 1. Communicate using LoRaWAN protocol.
 * 
 * HelTec AutoMation, Chengdu, China
 * 成都惠利特自动化科技有限公司
 * www.heltec.org
 *
 * this project also realess in GitHub:
 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
 * */

/* For LoRaWan func */
#include "LoRaWan_APP.h"

/* For display func */
#include <Wire.h>               
#include "HT_SSD1306Wire.h"
#include "images.h"

#include "esp32/rom/rtc.h"


/* Configuration of display */
SSD1306Wire  boardDisplay(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst



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

/*LoRaWan: Prepares the payload of the frame */
static void prepareTxFrame( uint8_t port )
{
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */
    appDataSize = 4;
    appData[0] = 0x00;
    appData[1] = 0x01;
    appData[2] = 0x02;
    appData[3] = 0x03;
}



RTC_DATA_ATTR bool firstRun = true;

void setup() {

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


}



void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}




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


void drawKioteraImage() {

  boardDisplay.clear();

  // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
  // on how to create xbm files
  boardDisplay.drawXbm(0, 20, kioteraLogo_width, kioteraLogo_height, kioteraLogo_bits);

  // write the buffer to the display
  boardDisplay.display();
  delay(1000);

}


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


//downlink data handle function example
void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
  Serial.printf("+REV DATA:%s,RXSIZE %d,PORT %d\r\n",mcpsIndication->RxSlot?"RXWIN2":"RXWIN1",mcpsIndication->BufferSize,mcpsIndication->Port);
  Serial.print("+REV DATA:");
  for(uint8_t i=0;i<mcpsIndication->BufferSize;i++)
  {
    Serial.printf("%02X",mcpsIndication->Buffer[i]);
  }
  Serial.println();
  uint32_t color=mcpsIndication->Buffer[0]<<16|mcpsIndication->Buffer[1]<<8|mcpsIndication->Buffer[2];
#if(LoraWan_RGB==1)
  turnOnRGB(color,5000);
  turnOffRGB();
#endif
}





// Aux String for printing in display
char str[30];


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
      LoRaWAN.generateDeveuiByChipID();
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



