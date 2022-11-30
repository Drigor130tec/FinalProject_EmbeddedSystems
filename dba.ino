#include <driver/i2s.h>
#include "simpleDSP.h"
#include "FFT.h"
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

#define DB_LIMIT 70
#define GAIN 20 //Gain for the dB's calibration

//MIC SETTINGS
#define SAMPLE_BUFFER_SIZE 2048 //Fs*0.5 seg, closest 2 power 
#define SAMPLE_RATE 8000

#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT
// either wire your microphone to the same pins or change these to match your wiring
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_23
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_22
#define I2S_MIC_SERIAL_DATA GPIO_NUM_21

// don't mess around with this
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

// and don't mess around with this
i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};

int32_t raw_samples[SAMPLE_BUFFER_SIZE];
//END OF MIC SETTINGS

//ARRAYS FOR PROCESSING AUDIO
float prev_audio[SAMPLE_BUFFER_SIZE];
float current_audio[SAMPLE_BUFFER_SIZE];
float sample_audio[SAMPLE_BUFFER_SIZE];
float fft_output[SAMPLE_BUFFER_SIZE];
float fft_input[SAMPLE_BUFFER_SIZE];


//FILTER SETTINGS
//Filter coefficients. Taken from MATLAB(adsgn)
float coefB[7] =
    {   0.255741125204258,
        -0.511482250408513,
        -0.255741125204266,
        1.022964500817038,
        -0.255741125204262,
        -0.511482250408513,
        0.255741125204257};

float coefA[7] =
    {   1,
        -4.019576181115834,
        6.189406442920701,
        -4.453198903544124,
        1.420842949621880,
        -0.141825473830305,
        0.004351177233495};

IIR iir1;
//END OF FILTER SETTIGNS

void setup()
{
    Serial.begin(115200);
    // Start up the I2S peripheral
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
    
    //Filter constructor
    iirInit(&iir1, 7, coefB, 7, coefA);
    Serial.println("IIR filter initiliaze finished");

    // PREVIOUS AUDIO
    // Read from the I2S device
    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, raw_samples, sizeof(int32_t) * SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
    int samples_read = bytes_read / sizeof(int32_t);
    
    for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++)
    {
        prev_audio[i] = iirFilt(&iir1,(float) raw_samples[i]/8388608);
    }
}

void loop()
{
    char print_buf[300];
    // Read from the I2S device
    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, raw_samples, sizeof(int32_t) * SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
    int samples_read = bytes_read / sizeof(int32_t);
  
    // Initialization of filter
    iirInit(&iir1, 7, coefB, 7, coefA);

    // Initialization of FFT
    fft_config_t *real_fft_plan = fft_init(SAMPLE_BUFFER_SIZE, FFT_REAL, FFT_FORWARD, fft_input, fft_output);

    // Create the sample vector
    for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++)
    {
        current_audio[i] = iirFilt(&iir1, (float) raw_samples[i]/8388608);
        if (i < SAMPLE_BUFFER_SIZE / 2){
          sample_audio[i] = prev_audio[i+SAMPLE_BUFFER_SIZE/2];
        }
        else{
          sample_audio[i] = current_audio[i-SAMPLE_BUFFER_SIZE/2];
        }
        prev_audio[i] = current_audio[i]; 
        real_fft_plan->input[i] = sample_audio[i];
    }   

    // Execute FFT, output buffer is automatically populated
    fft_execute(real_fft_plan);
    float sum = 0;
    for (int k = 1 ; k < real_fft_plan->size / 2 ; k++)
    {
        /*The real part of a magnitude at a frequency is followed by the corresponding imaginary part in the output*/
        float mag = sqrt(pow(real_fft_plan->output[2*k],2) + pow(real_fft_plan->output[2*k+1],2))/1;
        sum = sum + pow(mag,2);
    }
    sum = sum / 1;
    float p = ((float)1/(SAMPLE_BUFFER_SIZE/2))*sum;
    float db = 10*log(p)+GAIN;
    Serial.println(db);

    
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
        if (db > DB_LIMIT){
            // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
            // pieces of information in a channel. .
            int x = ThingSpeak.writeField(myChannelNumber, 2, db, myWriteAPIKey);
    
            if(x == 200)
            {
                Serial.print("Channel update successful. ");
                Serial.println(db);
            }
            else
            {
                Serial.println("Problem updating channel. HTTP error code " + String(x));
            }
            lastTime = millis();
        }    
         // Read from ThingSpeak
  
//        float y = ThingSpeak.readFloatField(StationChannelNumber, FieldNumber, myReadAPIKey);
//        int statusCode = ThingSpeak.getLastReadStatus();
//
//        if(statusCode == 200)
//        {
//            Serial.println(String(y));
//        }
//        else
//        {
//            Serial.println("Problem reading channel");
//        }
    }
    fft_destroy(real_fft_plan);
    
}
