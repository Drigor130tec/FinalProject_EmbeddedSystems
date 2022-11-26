#include <esp_wpa2.h>
#include "ThingSpeak.h"
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

WiFiClient  client;

// Insert your network credentials
const char* ssid = "Tec"; 
#define EAP_IDENTITY "a01275287@tec.mx"
#define EAP_PASSWORD "Oskar310802$"

// Writing credentials
unsigned long myChannelNumber = 1;
const char * myWriteAPIKey = "5IVATAVA6XG7PI2I";

// Reading credentials
unsigned long StationChannelNumber = 1950128;
unsigned long FieldNumber = 1;
const char * myReadAPIKey = "7JN3HEU1DRXH7BJK";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

// Write variable
float t;

void setup() 
{
    
    Serial.begin(115200);  //Initialize serial
    Serial.println(F("Running..."));
  
    // WPA2 enterprise magic starts here
    WiFi.disconnect(true);      
    WiFi.mode(WIFI_STA);   //init wifi mode
    Serial.printf("Connecting to WiFi: %s ", ssid);
    // esp_wifi_sta_wpa2_ent_set_ca_cert((uint8_t *)incommon_ca, strlen(incommon_ca) + 1);
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
    esp_wpa2_config_t configW = WPA2_CONFIG_INIT_DEFAULT();
    esp_wifi_sta_wpa2_ent_enable(&configW);
    // WPA2 enterprise magic ends here
    WiFi.begin(ssid);
  
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();  
  
    ThingSpeak.begin(client);  // Initialize ThingSpeak
}

void loop() 
{
    if ((millis() - lastTime) > timerDelay) 
    {
    
        // Connect or reconnect to WiFi
        if(WiFi.status() != WL_CONNECTED)
        {
            Serial.print("Attempting to connect");
            while(WiFi.status() != WL_CONNECTED)
            {
                WiFi.begin(ssid); 
                delay(5000);     
            } 
            Serial.println("\nConnected.");
        }

        // Value sent to ThingSpeak
        t = 1.0;
    
        // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
        // pieces of information in a channel. .
        int x = ThingSpeak.writeField(myChannelNumber, 2, t, myWriteAPIKey);

        if(x == 200)
        {
            Serial.println("Channel update successful.");
        }
        else
        {
            Serial.println("Problem updating channel. HTTP error code " + String(x));
        }
    
         // Read from ThingSpeak
  
        float y = ThingSpeak.readFloatField(StationChannelNumber, FieldNumber, myReadAPIKey);
        int statusCode = ThingSpeak.getLastReadStatus();

        if(statusCode == 200)
        {
            Serial.println(String(y));
        }
        else
        {
            Serial.println("Problem reading channel");
        }
        lastTime = millis();
    }
}
