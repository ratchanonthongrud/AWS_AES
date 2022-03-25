#include "SPIFFS.h" //Read file data from data folder
#include <WiFiClientSecure.h> //Installing Certificate
#include <PubSubClient.h> //MQTT PubSub
#include <DHT.h>  // library for getting data from DHT
#include "mbedtls/aes.h"
//#include <stdlib.h>

// Enter your WiFi ssid and password
const char* ssid = "realme 5 Pro"; //Provide your SSID
const char* password = "thongrud"; // Provide Password
const char* mqtt_server = "a1qyntg9vvfbt6-ats.iot.us-east-1.amazonaws.com"; // Relace with your MQTT END point
const int   mqtt_port = 8883;

String Read_rootca;
String Read_cert;
String Read_privatekey;
long lastMsg = 0;
int Value = 0;
int count = 1;

WiFiClientSecure espClient;
PubSubClient client(espClient);


void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
}

//Decrypt Function
void decrypt(unsigned char * chipherText, char * key, unsigned char * outputBuffer){
  mbedtls_aes_context aes;
  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_dec( &aes, (const unsigned char*) key, strlen(key) * 8 );
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char*)chipherText, outputBuffer);
  mbedtls_aes_free( &aes );
}

void callback(char* topic, byte* payload, unsigned int length) {  
  String topic1=topic;
  char * key;
  
  if(topic1 == "Test_AES128"){
    key = "abcdefghijklmnop";
  }
  else if(topic1 == "Test_AES192"){
    key = "abcdefghijklmnopqrstuvwx";
  }
  else if(topic1 == "Test_AES256"){
    key = "abcdefghijklmnopqrstuvwxyz123456";
  }
  int k,C,a;
  char A[4];
  unsigned char Msg[16];
  unsigned char decipheredTextOutput[16],AllDecryptMsg[length/3];
  String B;
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] : ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  a=(length/3)/16;  
  //Serial.println();
  //Serial.println(length);
  //Serial.println(a);
  for (int i = 0; i < a; i++) {
    for(int j=0;j<16;j++){
      k=(3*j)+1;
      //Serial.print((char)payload[k+(16*3*i)-1]);
      //Serial.print((char)payload[k+(16*3*i)]);
      //Serial.print((char)payload[k+(16*3*i)+1]);
      sprintf (A, "%c%c%c",(char)payload[k+(16*3*i)-1],(char)payload[k+(16*3*i)],(char)payload[k+(16*3*i)+1]);
      B=(String)A; 
      C=B.toInt();
      Msg[j]=(unsigned char)C;
      //Serial.print(Msg[j]);
    }   
    
    decrypt(Msg, key, decipheredTextOutput);
    for (int j = 0; j < 16; j++) {
      AllDecryptMsg[j+16*i] = decipheredTextOutput[j];
      //Serial.print((char)AllDecryptMsg[j+16*i]);
    }
  }
  Serial.print("\nDeciphered text: ");
  for (int j = 0; j < length/3; j++) {
      Serial.print((char)AllDecryptMsg[j]);
    }

  Serial.println();  
}
  

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("ei_out", "hello world");
      // ... and resubscribe
      client.subscribe("ei_in");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  delay(1000);
  //=============================================================
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  //=======================================
  //Root CA File Reading.
  File file2 = SPIFFS.open("/AmazonRootCA1.pem", "r");
  if (!file2) {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.println("Root CA File Content:");
  while (file2.available()) {
    Read_rootca = file2.readString();
    Serial.println(Read_rootca);
  }
  //=============================================
  // Cert file reading
  File file4 = SPIFFS.open("/cc0fce4902-certificate.pem.crt", "r");
  if (!file4) {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.println("Cert File Content:");
  while (file4.available()) {
    Read_cert = file4.readString();
    Serial.println(Read_cert);
  }
  //=================================================
  //Privatekey file reading
  File file6 = SPIFFS.open("/cc0fce4902-private.pem.key", "r");
  if (!file6) {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.println("privateKey File Content:");
  while (file6.available()) {
    Read_privatekey = file6.readString();
    Serial.println(Read_privatekey);
  }
  //=====================================================

  char* pRead_rootca;
  pRead_rootca = (char *)malloc(sizeof(char) * (Read_rootca.length() + 1));
  strcpy(pRead_rootca, Read_rootca.c_str());

  char* pRead_cert;
  pRead_cert = (char *)malloc(sizeof(char) * (Read_cert.length() + 1));
  strcpy(pRead_cert, Read_cert.c_str());

  char* pRead_privatekey;
  pRead_privatekey = (char *)malloc(sizeof(char) * (Read_privatekey.length() + 1));
  strcpy(pRead_privatekey, Read_privatekey.c_str());

  Serial.println("================================================================================================");
  Serial.println("Certificates that passing to espClient Method");
  Serial.println();
  Serial.println("Root CA:");
  Serial.write(pRead_rootca);
  Serial.println("================================================================================================");
  Serial.println();
  Serial.println("Cert:");
  Serial.write(pRead_cert);
  Serial.println("================================================================================================");
  Serial.println();
  Serial.println("privateKey:");
  Serial.write(pRead_privatekey);
  Serial.println("================================================================================================");

  espClient.setCACert(pRead_rootca);
  espClient.setCertificate(pRead_cert);
  espClient.setPrivateKey(pRead_privatekey);

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  delay(2000);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  
  client.loop();
  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
 
    if(client.subscribe("Test_AES128",0) == true){// subscribe Topic and returns true upon success
      //Serial.println("\nSubscribe Success!");
    }
    else{
      //Serial.println("\nSubscribe Failed!");
    }
    
    //================================================================================================
  }
}
