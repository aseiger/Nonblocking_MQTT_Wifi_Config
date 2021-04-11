#include <WiFi.h>
#include <PubSubClient.h>

#include <PubSubClient.h>

#include <strings_en.h>
#include <WiFiManager.h>

#include <EEPROM.h>

#define CONFIG_LED 25

WiFiManager wifi_manager;
WiFiClient esp_client;
PubSubClient client(esp_client);

/********************** Begin EEPROM Section *****************/
#define EEPROM_SALT 12664
typedef struct
{
  int   salt = EEPROM_SALT;
  char mqtt_server[256]  = "";
  char mqtt_port[6] = "";
} MQTTSettings;
MQTTSettings mqtt_settings;

WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_settings.mqtt_server, 256);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_settings.mqtt_port, 6);

void eeprom_read()
{
  EEPROM.begin(512);
  EEPROM.get(0, mqtt_settings);
  EEPROM.end();
}


void eeprom_saveconfig()
{
  EEPROM.begin(512);
  EEPROM.put(0, mqtt_settings);
  EEPROM.commit();
  EEPROM.end();
}

bool should_save_config = false;

/*********************************************************************************/

//callback notifying us of the need to save config
void saveConfigCallback () {
    should_save_config = true;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
  
  digitalWrite(CONFIG_LED, HIGH); //LED ON
}

void setup() {
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP 
    Serial.begin(115200);
    Serial.println("\n Starting");

    pinMode(CONFIG_LED, OUTPUT);

    eeprom_read();
    if (mqtt_settings.salt != EEPROM_SALT)
    {
      Serial.println("Invalid settings in EEPROM, trying with defaults");
      MQTTSettings defaults;
      mqtt_settings = defaults;
    }

    wifi_manager.resetSettings();
    wifi_manager.addParameter(&custom_mqtt_server);
    wifi_manager.addParameter(&custom_mqtt_port);
    wifi_manager.setConfigPortalBlocking(false);
    wifi_manager.setSaveParamsCallback(saveConfigCallback);
    wifi_manager.setAPCallback(configModeCallback);

    char setup_ssid[32];
    sprintf(setup_ssid, "MYDEVICE-%d", String(WIFI_getChipId(),HEX)); //change here 
    Serial.print("Setup SSID: ");
    Serial.println(setup_ssid);

    if(wifi_manager.autoConnect(setup_ssid)){
        Serial.println("connected...yeey :)");
    }
    else {
        Serial.println("Configportal running");
    }

}

void loop() {
  bool is_connected = wifi_manager.process();
  if (should_save_config & is_connected)
  {
    should_save_config = false;
    strcpy(mqtt_settings.mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_settings.mqtt_port, custom_mqtt_port.getValue());
    eeprom_saveconfig();
    
    Serial.print("New MQTT Server: ");
    Serial.println(mqtt_settings.mqtt_server);
    Serial.print("New MQTT Port: ");
    Serial.println(mqtt_settings.mqtt_port);
    
    digitalWrite(CONFIG_LED, LOW); //LED OFF 
  }
}
