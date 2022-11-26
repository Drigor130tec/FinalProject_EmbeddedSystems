/*
  Adapted from WriteSingleField Example from ThingSpeak Library (Mathworks)
  
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-thingspeak-publish-arduino/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/
#include <esp_wpa2.h>
#include "ThingSpeak.h"
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
//#include <Firebase_ESP_Client.h>
//#include <Adafruit_BME280.h>
//#include <Adafruit_Sensor.h>
#include "DHT.h"

#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

//const char* ssid = "Tec-IoT";   // your network SSID (name) 
//const char* password = "spotless.magnetic.bridge";   // your network password

WiFiClient  client;

// Insert your network credentials
const char* ssid = "Tec"; 
#define EAP_IDENTITY "a01275287@tec.mx"
#define EAP_PASSWORD "Oskar310802$"


unsigned long myChannelNumber = 1;
const char * myWriteAPIKey = "5IVATAVA6XG7PI2I";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

// Variable to hold temperature readings
float temperatureC;


void setup() 
{
    Serial.begin(115200);  //Initialize serial
    Serial.println(F("DHTxx test!"));

    dht.begin();
  
    // WPA2 enterprise magic starts here
    WiFi.disconnect(true);      
    WiFi.mode(WIFI_STA);   //init wifi mode
    Serial.printf("Connecting to WiFi: %s ", ssid);
    //esp_wifi_sta_wpa2_ent_set_ca_cert((uint8_t *)incommon_ca, strlen(incommon_ca) + 1);
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

        // Reading temperature or humidity takes about 250 milliseconds!
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        float h = dht.readHumidity();
        // Read temperature as Celsius (the default)
        float t = dht.readTemperature();
        // Read temperature as Fahrenheit (isFahrenheit = true)
        float f = dht.readTemperature(true);

        // Check if any reads failed and exit early (to try again).
//        if (isnan(h) || isnan(t) || isnan(f)) 
//        {
//            Serial.println(F("Failed to read from DHT sensor!"));
//            return;
//        }

        // Compute heat index in Fahrenheit (the default)
        float hif = dht.computeHeatIndex(f, h);
        // Compute heat index in Celsius (isFahreheit = false)
        float hic = dht.computeHeatIndex(t, h, false);

        Serial.print(F("Humidity: "));
        Serial.print(h);
        Serial.print(F("%  Temperature: "));
        Serial.print(t);
        Serial.print(F("°C "));
        Serial.print(f);
        Serial.print(F("°F  Heat index: "));
        Serial.print(hic);
        Serial.print(F("°C "));
        Serial.print(hif);
        Serial.println(F("°F"));
        t = 1.0;
    
        // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
        // pieces of information in a channel.  Here, we write to field 1.
        int x = ThingSpeak.writeField(myChannelNumber, 2, t, myWriteAPIKey);

        if(x == 200)
        {
            Serial.println("Channel update successful.");
        }
        else
        {
            Serial.println("Problem updating channel. HTTP error code " + String(x));
        }
        lastTime = millis();
    }
  
}
