#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi Credentials
const char* ssid = "Airtel_Aizah";
const char* password = "aizahJ0205";

// MQTT Setup (Updated for HiveMQ)
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "stocks/price";
const char* mqtt_client_id = "esp32_stock_tracker_123"; // Must be unique

WiFiClient espClient;
PubSubClient client(espClient);

// Stock symbols to track
const char* stocks[] = {"AAPL", "AMZN", "TSLA", "MSFT", "PFE", "OXY", "EBAY", "FDX"};
int stockIndex = 0;
unsigned long lastUpdate = 0;
const long updateInterval = 3000; // 3 seconds per stock

void setupDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED failed"));
    while(1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Stock Tracker");
  display.display();
  delay(2000);
}

void connectWiFi() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.print(".");
    display.display();
  }
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected!");
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);
}

void reconnectMQTT() {
  while (!client.connected()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Connecting MQTT...");
    display.display();
    
    if (client.connect(mqtt_client_id)) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("MQTT Connected!");
      display.display();
      delay(1000);
    } else {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Failed, rc=");
      display.println(client.state());
      display.println("Retry in 5s...");
      display.display();
      delay(5000);
    }
  }
}

void publishStockData(const String& symbol, float price, float change) {
  DynamicJsonDocument doc(256);
  doc["symbol"] = symbol;
  doc["price"] = price;
  doc["change"] = change;
  doc["unit"] = "USD";
  doc["timestamp"] = millis();
  
  String payload;
  serializeJson(doc, payload);
  
  client.publish(mqtt_topic, payload.c_str());
}

void displayStock(const String& symbol, float price, float change) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(symbol);
  
  display.setTextSize(1);
  display.setCursor(0, 25);
  display.print("Price: $");
  display.println(price, 2);
  
  display.setCursor(0, 40);
  display.print("Change: ");
  display.print(change, 2);
  display.println("%");
  
  display.display();
}

void getStockPrice(const String& symbol) {
  String url = "https://finnhub.io/api/v1/quote?symbol=" + symbol + "&token=d13tij1r01qs7gljqgj0d13tij1r01qs7gljqgjg";
  HTTPClient http;
  http.begin(url);
  
  int httpCode = http.GET();
  if (httpCode > 0) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getString());
    
    float prevClose = doc["pc"];
    float currentPrice = doc["c"];
    float changePercent = ((currentPrice - prevClose) / prevClose) * 100;
    
    displayStock(symbol, currentPrice, changePercent);
    publishStockData(symbol, currentPrice, changePercent);
    
    Serial.print(symbol);
    Serial.print(": $");
    Serial.print(currentPrice, 2);
    Serial.print(" (");
    Serial.print(changePercent, 2);
    Serial.println("%)");
  } else {
    Serial.println("HTTP Error");
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  setupDisplay();
  connectWiFi();
  
  client.setServer(mqtt_server, mqtt_port);
  reconnectMQTT();
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("System Ready");
  display.println("Broker: HiveMQ");
  display.display();
  delay(2000);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  if (millis() - lastUpdate >= updateInterval) {
    getStockPrice(stocks[stockIndex]);
    stockIndex = (stockIndex + 1) % (sizeof(stocks)/sizeof(stocks[0]));
    lastUpdate = millis();
  }
}
