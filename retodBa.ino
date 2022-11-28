#include <esp_wpa2.h>
#include <driver/i2s.h>
#include "ThingSpeak.h"
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

// Mic settings
#define SAMPLE_BUFFER_SIZE 512
#define SAMPLE_RATE 8000
// most microphones will probably default to left channel but you may need to tie the L/R pin low
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT
// either wire your microphone to the same pins or change these to match your wiring
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_26
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_22
#define I2S_MIC_SERIAL_DATA GPIO_NUM_21

i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};
​
i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};
​
int32_t raw_samples[SAMPLE_BUFFER_SIZE];

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

    // start up the I2S peripheral
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
  
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
    // read from the I2S device
    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, raw_samples, sizeof(int32_t) * SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
    int samples_read = bytes_read / sizeof(int32_t);
    // print the samples the serial channel.
    for (int i = 0; i < samples_read; i++)
    {
        Serial.printf("%ld\n", raw_samples[i]);
    }
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
