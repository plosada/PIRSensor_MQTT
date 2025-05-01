#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <Arduino.h>

#define SDA 21
#define SCL 22
#define SENSOR_PIN 5
#define BUTTON_PIN_BITMASK 0x20

const char* ssid = "Telecentro-5068";
const char* password = "MMZUGMMRTZYY";
const char* mqttServer = "192.168.0.12";
const int mqttPort = 1883;
const char* mqttUser = "admin";
const char* mqttPassword = "36168059";

long lastMsg = 0;

// Initialize the client
WiFiClient PIRnode1;
PubSubClient client(PIRnode1);

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}



void setup() {
  Serial.begin(115200);

  pinMode(SENSOR_PIN, INPUT); //pir sensor pin as input

  print_wakeup_reason();//Print the wakeup reason for ESP32

  Serial.println("Voltlog ESP32 PIR Node HW rev.B");

  setup_wifi(); // This initializes the wifi service.
   
  client.setServer(mqttServer, mqttPort);
  //client.setCallback(callback);

  //Configure GPIO2 as ext1 wake up source for HIGH logic level
  // esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);
  // esp_sleep_enable_ext0_wakeup(SENSOR_PIN,HIGH);
  esp_deep_sleep_enable_gpio_wakeup(BUTTON_PIN_BITMASK, ESP_GPIO_WAKEUP_GPIO_HIGH);

  int PIRValue = digitalRead(SENSOR_PIN);

  Serial.println(PIRValue);

  if(PIRValue == HIGH){
    publishPIR();  //if PIR sensor triggered send a msg
  }

  sleep();
}

void loop() {
  // put your main code here, to run repeatedly:

}

void sleep() {
  //Go to sleep now
  Serial.println("Going to sleep now..");
  esp_wifi_stop();  //turn off wifi
  esp_bt_controller_disable();         //turn off bt
  esp_deep_sleep_start();
}

void publishPIR() {
 if (!client.connected()) {
    reconnect();
  }
  client.loop();

  char buffer[2048];
  DynamicJsonDocument doc(2048);
  doc.clear();
  
  // A simple MQTT PIR Sensor
  doc["name"] = "MQTT PIR Sensor";
  doc["unique_id"] = "mqttpir01";
  doc["state_topic"] = "stat/mqtt/sensor/pir01";
  serializeJson(doc, buffer);

  client.publish("homeassistant/sensor/mqtt_pir_sensor/config", buffer, true);

  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;    
    // Publishes new data to server
    client.publish("stat/mqtt/sensor/pir01", String("on").c_str());
    Serial.println("ON sent to server");
  }
  delay(2000);//delay for 3 seconds to avoid consecutive triggers from deep sleep, PIR output pin stays high for 2-3 seconds

  client.publish("stat/mqtt/sensor/pir01", String("off").c_str());
  Serial.println("OFF sent to server");
  delay(1000);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("PIRnode1", mqttUser, mqttPassword )){
      Serial.println("connected");
      // Subscribe
      //client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}