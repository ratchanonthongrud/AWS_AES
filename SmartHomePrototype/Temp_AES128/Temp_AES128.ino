#include "SPIFFS.h" //Read file data from data folder
#include <WiFiClientSecure.h> //Installing Certificate
#include <PubSubClient.h> //MQTT PubSub
#include <DHT.h>  // library for getting data from DHT
#include "mbedtls/aes.h"

// Enter your WiFi ssid and password
const char* ssid = "realme 5 Pro"; //Provide your SSID
const char* password = "thongrud"; // Provide Password
const char* mqtt_server = "a1qyntg9vvfbt6-ats.iot.us-east-1.amazonaws.com"; // Relace with your MQTT END point
const int   mqtt_port = 8883;

String Read_rootca;
String Read_cert;
String Read_privatekey;
#define BUFFER_LEN 128
long lastMsg = 0;
char msg[BUFFER_LEN];
int Value = 0;
int count = 1;

WiFiClientSecure espClient;
PubSubClient client(espClient);



#define DHTPIN 4        //pin where the DHT22 is connected 
DHT dht(DHTPIN, DHT11);

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
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
  dht.begin();
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(2, OUTPUT);
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

//Encrypt Function
void encrypt(char * plainText, char * key, unsigned char * outputBuffer){
  mbedtls_aes_context aes;
  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_enc( &aes, (const unsigned char*) key, strlen(key) * 8 );
  mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)plainText, outputBuffer);
  mbedtls_aes_free( &aes );
}


void loop() {
  char * key = "abcdefghijklmnop"; //กำหนดขนาด key
  unsigned char cipherTextOutput[32];
  String Ciphertext; 
  String Ciphertext2; 
  String Zero = "0";
  
  float h = dht.readHumidity();   // Reading Temperature form DHT sensor
  float t = dht.readTemperature();      // Reading Humidity form DHT sensor
  float tF = (t * 1.8) + 32;
  if (isnan(h) || isnan(t))
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    //=============================================================================================
    String Temprature = String(t);
    String Humidity = String(h);
    snprintf (msg, BUFFER_LEN, "Temperature:%s,Humidity:%s", Temprature.c_str(), Humidity.c_str());
    
    char P[16];
    int a,b,c;
    
    if(strlen(msg)%16==0){
      a=strlen(msg)/16;
      c=a*16*3;
    }
    else{
      a=(strlen(msg)/16)+1;
      b=strlen(msg)%16;
      c=(a+1)*16*3;
    }

    for (int i = 0; i < a; i++) {
      
      if(i==a-1 && b>0){
        for(int j = 0; j < 16; j++){
          P[j]=msg[(16*i)+j]; 
        }
        encrypt(P, key, cipherTextOutput); //เรียกใช้ฟังก์ชัน Encrypt
        for (int i = 0; i < 16; i++) {    
           Ciphertext = cipherTextOutput[i];
           char payload[4]; //creat payload for publishing 
           Ciphertext.toCharArray(payload,4);
           //Serial.print(strlen(payload));
    
            if(strlen(payload)==1){
              Ciphertext2 += (Zero+Zero+payload);
            }
            else if(strlen(payload)==2){
              Ciphertext2 += (Zero+payload);
            }
            else {
              Ciphertext2 += payload;
            }
        }
      }
      
      else{
        for(int j = 0; j < 16; j++){
          P[j]=msg[(16*i)+j];
        }
        encrypt(P, key, cipherTextOutput); //เรียกใช้ฟังก์ชัน Encrypt
        for (int i = 0; i < 16; i++) {    
           Ciphertext = cipherTextOutput[i];
           char payload[4]; //creat payload for publishing 
           Ciphertext.toCharArray(payload,4);
           //Serial.print(strlen(payload));
    
            if(strlen(payload)==1){
              Ciphertext2 += (Zero+Zero+payload);
            }
            else if(strlen(payload)==2){
              Ciphertext2 += (Zero+payload);
            }
            else {
              Ciphertext2 += payload;
            }
        }
      }
    }

    char payload2[c+1]; //creat payload for publishing 
    Ciphertext2.toCharArray(payload2,c+1);
    
    Serial.print("Publish message: ");
    Serial.print(count);
    Serial.print(" ");
    Serial.println(msg);
    
    Serial.println("Ciphered text: "); 
    Serial.print(Ciphertext2);
    
    if(client.publish("Temp_AES128", payload2) == true){// Publishes payload and returns true upon success
      Serial.println("\nPublish Success!");
    }
    else{
      Serial.println("\nPublish Failed!");
    }

    count = count + 1;
    Serial.print("\n");
    //================================================================================================
  }
 }
