#include <ESP8266WiFi.h>
//MQTT
#include <PubSubClient.h>

const char* ssid     = "Soares";     // Your ssid  
const char* password = "soares2020"; // Your Password  
const char* testHostname = "www.google.com";  
IPAddress HostIP;  
unsigned int localPort = 80;  
const int RELAY_Pin = 14;               //Relay Pin

//MQTT CONFIG
const String HOSTNAME  = "RouterNOS"; //NOME DO DEVICE, deverá ter um nome unico.
const char * MQTT_COMMAND_TOPIC = "testeRouter/set"; //Topico onde o Device subscreve.
const char * MQTT_STATE_TOPIC = "testeRouter/state"; //Topico onde o Device publica.
const char* MQTT_SERVER = "luisfosoares.ddns.net"; //IP ou DNS do Broker MQTT

// Credrenciais ao broker mqtt. Caso nao tenha AUTH meter a false.
#define MQTT_AUTH true
#define MQTT_USERNAME "luisfosoares"
#define MQTT_PASSWORD "raspberry"

WiFiClient wclient;
PubSubClient client(MQTT_SERVER, 1883, wclient);

#define MINUTES (60L * 1000)
#define SECONDS  1000
const unsigned long  PROBE_DELAY = 1 * MINUTES;  
const unsigned long  RESET_DELAY = 4 * MINUTES;  
const unsigned long  RESET_PULSE = 2 * SECONDS;  
int  Nreset_events = 0;
int Ndown = 0;

enum {  
    TESTING_STATE=0, FAILURE_STATE=1, SUCCESS_STATE=2
};

int CurrentState = TESTING_STATE;

bool checkMqttConnection() {
Serial.println("Checking mqtt connection\n");

if (!client.connected()) {
if (MQTT_AUTH ? client.connect(HOSTNAME.c_str(), MQTT_USERNAME, MQTT_PASSWORD) : client.connect(HOSTNAME.c_str())) {
    Serial.println("Ligado ao broker mqtt " + String(MQTT_SERVER));
    //SUBSCRIÇÃO DE TOPICOS
    client.subscribe(MQTT_COMMAND_TOPIC);
     }
    }
return client.connected();
   }
   
void setup() {  
    pinMode(RELAY_Pin, OUTPUT);

    Serial.begin(115200);
    delay(10);
    Serial.println( __FILE__ );
    delay(10);

    // Connecting to a WiFi network
    Serial.print(String("Connecting to ") + ssid);

    WiFi.begin(ssid, password);


    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nWiFi connected");
    if (WiFi.status() == WL_CONNECTED) {
    if (checkMqttConnection()) {
      
      client.loop();

     }
    }
}



void reset_device() {  
    // keep track of number of resets
    Nreset_events++;
    Serial.println(String("\nDisconnected... resetting - ") + String(Nreset_events));
    digitalWrite(RELAY_Pin, HIGH);
    delay(RESET_PULSE);
    digitalWrite(RELAY_Pin, LOW);
}

void loop() {  
    if (WiFi.status() == WL_CONNECTED) {
    if (checkMqttConnection()) {
      
      client.loop();
     }
    }
    
    switch (CurrentState) {

        case TESTING_STATE:
            if (!WiFi.hostByName(testHostname, HostIP)) {
                CurrentState = FAILURE_STATE;
            } else {
                CurrentState = SUCCESS_STATE;
            }
            break;

        case FAILURE_STATE:
            //Serial.print(String("Router Reset\n"));
            reset_device();
            delay(RESET_DELAY);
            CurrentState = TESTING_STATE;
            //client.publish(MQTT_STATE_TOPIC, "Router Reset");
            //client.publish(MQTT_STATE_TOPIC, "Resets:%d "+ Nreset_events);
            break;

        case SUCCESS_STATE:
        
            Serial.print(String(Nreset_events) + "\n");
            Serial.print(String(Ndown) + "\n");

            
            if (Nreset_events != Ndown){
              Serial.print(String("Router Reset\n"));
              client.publish(MQTT_COMMAND_TOPIC, "Router              Reset");  
              
              char charBuf[String(Nreset_events).length() + 1];
              String(Nreset_events).toCharArray(charBuf,String(Nreset_events).length() + 1);
              client.publish(MQTT_COMMAND_TOPIC, charBuf);
             
              
              Ndown++;
            }
            
            Serial.print(String("Router OK\n"));
            client.publish(MQTT_STATE_TOPIC, "Router Ok");
    
   

            delay(PROBE_DELAY);
            CurrentState = TESTING_STATE;
            break;
    }
}
