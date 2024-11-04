#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h> 

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// MQTT broker information
const char* mqtt_server = "f69f6416905b4890a65c0d638608cfff.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "test";  
const char* mqtt_password = "1";

WiFiClientSecure wifiClient;  
PubSubClient client(wifiClient);

bool ledState = false;
bool isWifiConnected = false; // Track Wi-Fi connection status

void connectToMqtt();  // Forward declaration
void publishLedState(); // Forward declaration for publishing LED state

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue().c_str();
      
      if (value.length() > 0) {
        Serial.print("Received Value: ");
        Serial.println(value);

        if (value.startsWith("ssid:") && value.indexOf(",psw:") != -1) {
          int ssidStart = value.indexOf("ssid:") + 5;
          int pswStart = value.indexOf(",psw:") + 5;
          String ssid = value.substring(ssidStart, pswStart - 5);
          String password = value.substring(pswStart);

          Serial.println("Connecting to Wi-Fi...");
          WiFi.begin(ssid.c_str(), password.c_str());
          
          int attempts = 0;
          while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
          }

          if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected to Wi-Fi!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            
            isWifiConnected = true;  // Set Wi-Fi status to connected
          } else {
            Serial.println("\nFailed to connect to Wi-Fi");
            isWifiConnected = false;
          }
        } else if (value == "1") {
          ledState = true;
          digitalWrite(LED_BUILTIN, LOW);
          Serial.println("LED ON");
          publishLedState(); // Publish the LED state
        } else if (value == "0") {
          ledState = false;
          digitalWrite(LED_BUILTIN, HIGH);
          Serial.println("LED OFF");
          publishLedState(); // Publish the LED state
        } else {
          Serial.println("Invalid value received");
        }
      }
    }
};

// MQTT callback function to handle incoming messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Connect to MQTT broker function
void connectToMqtt() {
  client.setServer(mqtt_server, mqtt_port);

  Serial.print("Connecting to MQTT...");
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      
      // Subscribe to a topic
      client.subscribe("hello");  
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// Function to publish the LED state
void publishLedState() {
  if (isWifiConnected && client.connected()) {
    // Read the current state of the LED
    int currentLedState = digitalRead(LED_BUILTIN);
    String ledMessage = (currentLedState == HIGH) ? "LED is OFF" : "LED is ON";
    
    client.publish("hello", ledMessage.c_str());
    Serial.print("Published: ");
    Serial.println(ledMessage);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Ensure LED is off at startup

  // Set up BLE
  BLEDevice::init("XIAO_ESP32S3");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Hello World");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read/write it in your phone!");

  wifiClient.setInsecure();  // Set up insecure connection for testing
  client.setCallback(mqttCallback);  // Set the MQTT callback function
}

void loop() {
  if (isWifiConnected) {
    if (!client.connected()) {
      connectToMqtt();  // Only attempt MQTT connection if Wi-Fi is connected
    }
    client.loop();  // Maintain MQTT connection
  } else {
    // Attempt Wi-Fi reconnection if necessary
    if (WiFi.status() != WL_CONNECTED) {
      isWifiConnected = false;
      Serial.println("Wi-Fi disconnected. Waiting for new credentials...");
    }
  }
  delay(2000);
}
