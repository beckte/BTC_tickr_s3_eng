// 1. Include necessary libraries
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

// ★★★ Use library for T-Display S3 ★★★
#include <TFT_eSPI.h> // Hardware-specific library

// --- API and Time Settings ---
String btcApiUrl = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd&include_24hr_change=true";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // GMT+7 for Thailand
const int   daylightOffset_sec = 0;

// ★★★ TFT_eSPI screen setup for T-Display S3 ★★★
TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

// ★★★ Define custom grey color (if TFT_GREY is not declared) ★★★
#define CUSTOM_GREY 0x8410 // RGB (128, 128, 128) approximately

void setup() {
  Serial.begin(115200);

  // --- WiFiManager Section ---
  WiFiManager wm;
  bool res = wm.autoConnect("BTC-Ticker-Setup");
  if (!res) {
    Serial.println("Failed to connect Wi-Fi, restarting...");
    ESP.restart(); 
  } else {
    Serial.println("WiFi Connected!");
  }

  // ★★★ Initialize TFT screen once in setup() ★★★
  tft.begin(); 
  tft.setRotation(1); // Set screen rotation (0, 1, 2, 3 - adjust as needed for horizontal/vertical)
  tft.fillScreen(TFT_BLACK); // Start with a black background 
  
  // Display initial data immediately
  updateDisplayData();
}

void loop() {
  // --- Variables for controlling data update on screen ---
  static unsigned long lastUpdateTime = 0;
  const long updateInterval = 2 * 60 * 1000; // Update every 2 minutes

  unsigned long currentTime = millis();

  // Check if it's time to update data
  if (currentTime - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentTime; 

    updateDisplayData(); // Call function to update data and display
  }
}

// ★★★ Function to fetch and display data on screen ★★★
void updateDisplayData() {
  Serial.println("\n--- Updating data ---");

  // --- 2. Fetch time data from NTP Server ---
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); 
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  } else {
    Serial.print("Current time: ");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  }

  // --- 3. Fetch price and % Change data from Coingecko API ---
  String btcPriceUSD = "N/A";
  float change24h = 0.0;
  HTTPClient http;
  http.begin(btcApiUrl);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    
    float price = doc["bitcoin"]["usd"];
    // Remove decimal places from BTC price
    btcPriceUSD = String(price, 0); 
    
    change24h = doc["bitcoin"]["usd_24h_change"];
    Serial.print("Bitcoin Price: $");
    Serial.println(btcPriceUSD);
    Serial.print("24h Change: ");
    Serial.print(change24h, 2);
    Serial.println("%");
  } else {
    Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end(); 
  
  // --- 4. Display on TFT screen (T-Display S3) ---
  tft.fillScreen(CUSTOM_GREY); 
  tft.fillRect(0, 0, 85, tft.height(), TFT_BLACK); 

  // 4.2 Display "BTC" in the black bar
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(4); 
  tft.setCursor(20, 75); 
  tft.print("BTC");

  // Prepare String for time display
  char timeString[40];
  strftime(timeString, sizeof(timeString), "%I:%M%p %a %d %b %Y", &timeinfo);
  String finalTimeString = String(timeString);
  finalTimeString.toUpperCase(); 

  // Prepare String for % Change display with color
  String changeString = "1 day ";
  if (change24h >= 0) {
    changeString += "+";
    tft.setTextColor(TFT_GREEN); 
  } else {
    tft.setTextColor(TFT_RED); 
  }
  changeString += String(change24h, 2) + "%";
  
  // Display % change (bottom)
  tft.setTextFont(2); 
  tft.setCursor(95, 135);
  tft.print(changeString);

  // Change text color to black for the rest of the display
  tft.setTextColor(TFT_BLACK);

  // Display date and time (top)
  tft.setTextFont(2); 
  tft.setCursor(95, 20);
  tft.print(finalTimeString);
  
  // ★★★ Display dollar symbol separately with TFT_FONT4 ★★★
  tft.setTextFont(4); // Use a medium font that supports the dollar symbol
  tft.setCursor(100, 80); // Adjust position as needed for price
  tft.print("$");

  // ★★★ Display price (adjust position to align with $) ★★★
  tft.setTextFont(7); // Use TFT_FONT7 for the price number
  // Calculate x position to align with the dollar symbol
  tft.setCursor(100 + tft.textWidth("$"), 65); // x position: 100 + width of '$'
  tft.print(btcPriceUSD);
}