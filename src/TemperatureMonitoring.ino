// Include necessary libraries
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <SimpleTimer.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

// Define Wi-Fi credentials
const char *ssid="YOUR_USERNAME";
const char *pass="YOUR_PASSWORD";

// Define host for IFTTT communication
const char* host = "maker.ifttt.com";

// Initialize objects and variables
SimpleTimer timer;
RTC_DS3231 rtc;
WiFiClient client;
DateTime now;
#define ONE_WIRE_BUS 2 // DS18B20 on arduino pin2 corresponds to D4 on physical board "D4 pin on the ndoemcu Module"
char auth[] = BLYNK_AUTH_TOKEN;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2); 
float temp;
float Fahrenheit=0;
bool alerted = false;
unsigned long lastAlertTime = 0;
unsigned long alertInterval = 300000;

// Setup function
void setup() {
  // Set up LED pins
  pinMode(D6,OUTPUT); //Led pin which shows the status of wifi
  pinMode(D7,OUTPUT); //Led pin which shows the status of temperature
  pinMode(D8,OUTPUT); //Led pin which shows the status of temperature
  digitalWrite(D6,LOW);
  digitalWrite(D7,LOW);
  digitalWrite(D8,LOW);

  // Initialize LCD display
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Temperature:");
  WiFi.begin(ssid, pass);

  // Connect to Wi-Fi
  while((!(WiFi.status() == WL_CONNECTED)))
    {
      connectWiFi();//Wfi connecting function call
    }  

  // Initialize Serial communication  
  Serial.begin(115200);

  // Initialize DS18B20 temperature sensor
  DS18B20.begin();

  // Initialize RTC
  rtc.begin();

  // Initialize Blynk
  Blynk.begin(auth, ssid, pass);

  // Set up timer for periodic data sending
  timer.setInterval(1000L, getSendData);
}

// Main loop function
void loop() {
   Blynk.run();
   timer.run();
}

// Function to get and send temperature data
void getSendData()
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connection lost. Reconnecting...");
        digitalWrite(D6,LOW);
        connectWiFi();
        Blynk.begin(auth, ssid, pass);
    }
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
       Serial.println("Connection failed");
       return;
    }
    // Update LCD display with temperature
    lcd.setCursor(0, 1);
    lcd.print("   ");
    lcd.setCursor(3, 1);
    DS18B20.requestTemperatures(); //Get temperature from the sensor
    temp = DS18B20.getTempCByIndex(0);//converting temperature value to Celcius
    lcd.print(temp,3);
    Fahrenheit = DS18B20.toFahrenheit(temp);//converting temperature value to Fahrenheit
    Serial.println(temp);
    Blynk.virtualWrite(V0, temp);//Sending the live data to blynk app via virtual pin
    Serial.println(Fahrenheit);

    // Temperature alert logic
    if (temp > 8.0 && !alerted) {
        alerted = true;
        digitalWrite(D8,LOW);
        digitalWrite(D7,HIGH);
        sendSms(temp);//Function call for sending sms
        lcd.clear();
        lcd.print("Alert Sent");
        delay(1000);
        lcd.clear();
        lcd.print("Temperature:");
    }else if (temp <= 6.0) {
       digitalWrite(D7,LOW);
       digitalWrite(D8,HIGH);
       alerted = false;
  }

  // Reminder alert logic
  if (now.hour() >= 8 && now.hour() <= 20) {
   if (now.minute() % 30 == 0 && !alerted) {
    alerted = true;
    lcd.clear();
    lcd.print("Reminder Alert Sent");
  }
}else if (temp <= 6.0) {
  alerted = false;
}
Serial.println(alerted);
  delay(1000);
}

// Function to connect to Wi-Fi
void connectWiFi()
{ 
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  digitalWrite(D6,HIGH);
}

// Function to send SMS via IFTTT
void sendSms(float temp)
  {
    Serial.println(temp);
    String url = "/trigger/Temperature_Alert/with/key/b6wFi44jPYDg74xTl8I_m3Y08RHO_CLuSpCLqM8aeNQ/?value1=" + (String)temp;
    Serial.print("Requesting URL: ");
    Serial.println(url);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
  }
