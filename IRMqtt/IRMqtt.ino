/*
 * MQTT IR server, receiver
 *
 * used library:
 * https://github.com/markszabo/IRremoteESP8266
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <PubSubClient.h>

/*   
     Sonoff SV (ESP8266)
     GPIO_KEY1,        // GPIO00 Button
     GPIO_USER,        // GPIO01 Serial RXD and Optional sensor
     0,                // GPIO02 gong sensor, pullUP 1K8
     GPIO_USER,        // GPIO03 Serial TXD and Optional sensor
     GPIO_USER,        // GPIO04 Optional sensor
     GPIO_USER,        // GPIO05 Optional sensor
     0, 0, 0, 0, 0, 0, // Flash connection
     GPIO_REL1,        // GPIO12 Red Led and Relay (0 = Off, 1 = On)
     GPIO_LED1_INV,    // GPIO13 Green Led (0 = On, 1 = Off)
     GPIO_USER,        // GPIO14 Optional sensor
     0, 0,
     GPIO_ADC0         // ADC0 Analog input
 */

int RECV_PIN = 2;    // an IR detector/demodulator is connected to GPIO pin 0
int TRANS_PIN = 0;   // button -> send-ir

IRrecv irrecv(RECV_PIN);
IRsend irsend(TRANS_PIN);

const char* ssid = "myWLAN";
const char* password = "1234567890";

const char* topicRaportPrefix = "smarthome/ir/info/";
/* structure "smarthome/ir/sender/type[/bits[/address]]" */
const char* topicSubscribe = "smarthome/ir/sender/#";
const char* topicPrefix = "smarthome/ir/";

const char* mqtt_server = "192.168.178.10";
const char* mqtt_user = "ESP8266-ir";
const char* mqtt_pass = "mqtt_pass";

//String clientName;       // MQTT client name
char message_buff[100];

WiFiClient wifiClient;


void callback(char* topic, byte* payload, unsigned int length);
void connect_to_MQTT();

PubSubClient client(mqtt_server, 1883, callback, wifiClient);

// -----------------------------------------------------------------

void callback(char* topic, byte* payload, unsigned int length) {
  int i = 0;

  Serial.println("Message arrived:  topic: " + String(topic));
  Serial.println("Length: " + String(length,DEC));

  // create character buffer with ending null terminator (string)
  for(i=0; i<length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  
  unsigned int freq=38;
  String msgString = String(message_buff);
  String msgTopic = String(topic);
  unsigned long msgInt = msgString.toInt();
 
  Serial.println("Payload String: " + msgString);
  
  // structure "smarthome/ir/sender/type[/bits[/address]]"
  int endOfBits, endOfTyp;
  String irTypStr = "";
  String irBitsStr = "";
  int irBitsInt=-1;
  String irPanasAddrStr = "";
  
  endOfBits = msgTopic.indexOf("/",0); // vor ir
  if (endOfBits > 0)
    endOfTyp = msgTopic.indexOf("/",endOfBits+1); // vor sender
  if (endOfTyp > 0)
    endOfBits = msgTopic.indexOf("/",endOfTyp+1); // vor typ
  if (endOfBits > 0) 
     endOfTyp = msgTopic.indexOf("/",endOfBits+1); // nach typ
  if (endOfTyp == -1)
  {
    // One element - only irTyp
    irTypStr  = msgTopic.substring(endOfBits);
  } else {
    // irTyp exists and something more
    irTypStr  = msgTopic.substring(endOfBits+1, endOfTyp);
    endOfBits = msgTopic.indexOf("/",endOfTyp+1);
    if (endOfBits== -1)
    {
      // irBits is last
      irBitsStr = msgTopic.substring(endOfTyp+1);
    } else {
      // irBits and something more
      irBitsStr = msgTopic.substring(endOfTyp+1, endOfBits);
      irPanasAddrStr = msgTopic.substring(endOfBits+1);
    }
    irBitsInt = irBitsStr.toInt();
  }

  Serial.println("Type: " + irTypStr);
  Serial.println("Bits: " + irBitsStr);
  if (irPanasAddrStr != "")
    Serial.println("Addr: " + irPanasAddrStr);
  if (irTypStr=="NEC") {
    Serial.print("Send NEC:");
    Serial.println(msgInt);
    irsend.sendNEC(msgInt, 36);
  } else if (irTypStr=="RC5") {
    Serial.print("Send RC5:");
    Serial.print(msgInt);
    Serial.print(" (");
    Serial.print(irBitsInt);
    Serial.println("-bits)");
    irsend.sendRC5(msgInt, irBitsInt);
  } else {
    Serial.println("Wrong ir Type!");    
  }
    
}

// -----------------------------------------------------------------
String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

// -----------------------------------------------------------------
void setup(void)
{
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);

  irsend.begin();
  irrecv.enableIRIn();  // Start the receiver
  Serial.println("Ready to send and receive IR signals");

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  connect_to_MQTT();
}

// -----------------------------------------------------------------
void connect_to_MQTT() 
{
  Serial.print("Connecting to ");
  Serial.print(mqtt_server);
  Serial.print(" as ");
  //Serial.println(clientName);
  Serial.println(mqtt_user);
  char myTopic[100];
  
  int is_conn = 0;
  while (is_conn == 0) {
    if (client.connect(mqtt_user)) {
      //if (client.connect((char*) clientName.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Connected to MQTT broker");
      sprintf(myTopic, "%sclient", topicRaportPrefix);
      client.publish((char*)myTopic, mqtt_user);
      IPAddress myIp = WiFi.localIP();
      char myIpString[24];
      sprintf(myIpString, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
      sprintf(myTopic, "%sip", topicRaportPrefix);
      client.publish((char*)myTopic, (char*) myIpString);
      sprintf(myTopic, "%stype", topicRaportPrefix);
      client.publish((char*)myTopic,"IR server");
      Serial.print("Topic is: ");
      Serial.println(topicSubscribe);
      if (client.subscribe(topicSubscribe)){
        Serial.println("Successfully subscribed");
      }

      is_conn = 1;
    }
    else {
      Serial.print("MQTT connect failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// -----------------------------------------------------------------
// Encodig of data
void  encoding (decode_results *results, char * result_encoding)
{
  switch (results->decode_type) {
    default:
    case UNKNOWN:      strncpy(result_encoding,"UNKNOWN\0",8);       break ;
    case NEC:          strncpy(result_encoding,"NEC\0",4);           break ;
    case SONY:         strncpy(result_encoding,"SONY\0",5);          break ;
    case RC5:          strncpy(result_encoding,"RC5\0",4);           break ;
    case RC6:          strncpy(result_encoding,"RC6\0",4);           break ;
    case DISH:         strncpy(result_encoding,"DISH\0",5);          break ;
    case SHARP:        strncpy(result_encoding,"SHARP\0",6);         break ;
    case JVC:          strncpy(result_encoding,"JVC\0",4);           break ;
    case SANYO:        strncpy(result_encoding,"SANYO\0",6);         break ;
    case MITSUBISHI:   strncpy(result_encoding,"MITSUBISHI\0",11);   break ;
    case SAMSUNG:      strncpy(result_encoding,"SAMSUNG\0",8);       break ;
    case LG:           strncpy(result_encoding,"LG\0",3);            break ;
    case WHYNTER:      strncpy(result_encoding,"WHYNTER\0",8);       break ;
    case AIWA_RC_T501: strncpy(result_encoding,"AIWA_RC_T501\0",13); break ;
    case PANASONIC:    strncpy(result_encoding,"PANASONIC\0",11);    break ;
  }
}

//+=============================================================================
// Code to string
//
void fullCode (decode_results *results, char *myTmp)
{
  Serial.print("One line: ");
  Serial.print(myTmp);
  Serial.print(":");
  Serial.print(results->bits, DEC);
  if (results->repeat) {
    Serial.println(" (Repeat)");
    return;
  }
  if (results->overflow) {
    Serial.println("WARNING: IR code too long. "
                   "Edit IRController.ino and increase captureBufSize");
    return;    
  }
  Serial.print(":");  
  serialPrintUint64(results->value, 16);
  Serial.print(":");
  Serial.print(results->address, HEX);
  Serial.print(":");
  Serial.print(results->command, HEX);
  
  Serial.println("");
}

//+=============================================================================
// Dump out the decode_results structure.
//
void dumpCode(decode_results *results, char *myTmp) 
{
  // Start declaration
  Serial.print("uint16_t  ");              // variable type
  Serial.print("rawData[");                // array name
  Serial.print(results->rawlen - 1, DEC);  // array size
  Serial.print("] = {");                   // Start declaration

  // Dump data
  for (uint16_t i = 1; i < results->rawlen; i++) {
    Serial.print(results->rawbuf[i] * kRawTick, DEC);
    if (i < results->rawlen - 1)
      Serial.print(",");  // ',' not needed on last one
    if (!(i & 1)) Serial.print(" ");
  }

  // End declaration
  Serial.print("};");  //

  // Comment
  Serial.print("  // ");
  Serial.print(myTmp);
  Serial.print(" ");
  serialPrintUint64(results->value, 16);

  // Newline
  Serial.println("");
}

// -----------------------------------------------------------------
char myTopic[100];
char myType[50];
char myValue[100];
decode_results  myResults;        // Somewhere to store the results

void loop(void)
{
  /* this function will listen for incomming 
  subscribed topic-process-invoke receivedCallback */
  client.loop();

  if (! client.connected()) {
    Serial.println("Not connected to MQTT....");
    connect_to_MQTT();
  }
  
  if (irrecv.decode(&myResults)) {  // Grab an IR code
    encoding (&myResults, myType);
    
    fullCode(&myResults, myType);   // Print the singleline value
    //dumpCode(&myResults);         // Output the results as source code

    if (myResults.decode_type != UNKNOWN) {
      sprintf(myTopic, "%sreceiver/%s/%d"
            , topicPrefix, myType, myResults.bits );
      // any other has code and bits
      sprintf(myValue, "%d,", myResults.value );
      sprintf(myType, "%d,", myResults.address );
      strcat(myValue, myType);
      sprintf(myType, "%d", myResults.command );
      strcat(myValue, myType);

      /*
       * smarthome/ir/receiver/RC5/12 - 3467,22,11
       * smarthome/ir/receiver/NEC/32 - 16754775,0,21
       */
      client.publish((char*) myTopic, (char*) myValue );
    }
    
    Serial.print("Topic: ");
    Serial.print(myTopic);
    if (! myResults.repeat) {
      Serial.print(" - ");
      Serial.print(myValue);      
    }
    Serial.println("");

    irrecv.resume();              // Prepare for the next value
  }
} 
