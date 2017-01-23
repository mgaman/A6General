/*
 *   Using GPRS A6 development board, powewr comes from USB so no need to supply from Arduino
 *   RX/TX always connect to a hardware Serial port so we can utilize serialevent
 *   PWR always connected to Arduino VCC, RESET to TRANSISTOR_CONTROL
 *   
 *   Simple example publishes and subscribes to the same topic - see testTopic
 *   If using a public broker, choose a unique topic so you dont get swamped by other peoples stuff
 */
#include "A6Services.h"
#include "A6MQTT.h"

char buff[100];    // We must send at least 1 packet within the chosen keepalive time
                  // If you have nothing else to send, send at least a  ping
#define APN "uinternet"  // write your APN here
A6MQTT MQTT(30);
uint32_t nextpublish;
char topic[30];
char imei[20];
#define PUB_DELTA 25000 // publish every 20 secs

#define DEBUG_SERIAL Serial
void setup() {
  DEBUG_SERIAL.begin(115200);
   // A6 uses default baud 115200
   // power up the board, do hardware reset & get ready to execute commands
   DEBUG_SERIAL.println("A6 Simple MQTT");
  if (gsm.begin()) 
  {
    DEBUG_SERIAL.println("GSM up");
    // we need a unique userid when logging on to the broker. We also need a unique topic
    // name. We'll use this devices IMIE tp get that.
    if (!gsm.getIMEI(imei))
      strcpy(imei,"defaultime");
    strcpy(topic,imei);
    strcat(topic,"/test");
    // setup GPRS connection with your provider
    if (gsm.startIP(APN))
    {
      DEBUG_SERIAL.println("IP up");
      // AutoConnect sets up TCP session with the broker and makes a user connection
      MQTT.AutoConnect();
    }
    else
      DEBUG_SERIAL.println("IP down");
  }
  else
    DEBUG_SERIAL.println("GSM down");
}
void loop() {
  /*
   *  THe condition below checks if the modem is able to process data from the broker
   */
  if (MQTT.connectedToServer || MQTT.waitingforConnack)
  {
    /*
     * This machanism ensures that the connection with the broker is not disconnected du to inactivirt
     */
    if (MQTT._PingNextMillis < millis())
      MQTT.ping();
    /*
     *  Publish a message periodically
     */
    if (nextpublish < millis())
    {
      nextpublish = millis()+PUB_DELTA;
      sprintf(buff,"%lu",millis());
      MQTT.publish(topic,buff); // dup false, retain false, QOS 
    }
    /*
     * Process data received from the broker
     * Note that this may be mixed up with unsolicited messages from the modem. Parse takes car of that
     */
    MQTT.Parse();
  }
}

