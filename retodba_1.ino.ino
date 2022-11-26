#include <esp_wpa2.h>
#include "ThingSpeak.h"
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

WiFiClient  client;

// Insert your network credentials
const char* ssid = "Tec"; 
#define EAP_IDENTITY "a01275287@tec.mx"
#define EAP_PASSWORD "Oskar310802$"


unsigned long myChannelNumber = 1;//Thingspeak número de canal
const char * myWriteAPIKey = "5IVATAVA6XG7PI2I";// ThingSpeak write API Key

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

void setup() 
{
    Serial.begin(115200);  //Initialize serial
    Serial.println(F("Running..."));

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

        //Valor dummy enviado por la Esp32
        t = 1.0;

        // Carga los valores a enviar
        ThingSpeak.setField(2, t);

        // Escribe todos los campos a la vez.
        ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

        Serial.println("¡Datos enviados a ThingSpeak!");

        // Añadimos un retraso para limtitar el número de escrituras en Thinhspeak

        int duracionDelay = 300; //En segundos
        for (int i = 0; i < duracionDelay; i ++) { //Esto es debido a que el máximo que el Arduino puede procesar con precisión es 5000ms o 5 segundos
        delay (1000);
        }
    
        // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
        // pieces of information in a channel.  Here, we write to field 1.
       /* int x = ThingSpeak.writeField(myChannelNumber, 2, t, myWriteAPIKey);

        if(x == 200)
        {
            Serial.println("Channel update successful.");
        }
        else
        {
            Serial.println("Problem updating channel. HTTP error code " + String(x));
        }
        lastTime = millis();*/
    }
  
}
