#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"

const char* ssid = "YOUR_SSID";
const char* password = "PASSWORD";
const char* chatgpt_token = "CHATGPT_API/TOKEN";
const char* temperature = "0";
const char* max_tokens = "45";
String Question = "";

#define I2S_DOUT      25
#define I2S_BCLK      27
#define I2S_LRC       26

Audio audio;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  while (!Serial);

  // Wait for WiFi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(100);
}

void loop() {
  Serial.print("Ask your Question : ");
  while (!Serial.available()) {
    audio.loop();
  }
  while (Serial.available()) {
    char add = Serial.read();
    Question += add;
    delay(1);
  }
  int len = Question.length();
  Question = Question.substring(0, (len - 1));
  Question = "\"" + Question + "\"";
  Serial.println(Question);

  HTTPClient https;

  if (https.begin("https://api.openai.com/v1/chat/completions")) {  // HTTPS
    https.addHeader("Content-Type", "application/json");
    String token_key = "Bearer " + String(chatgpt_token); 
    https.addHeader("Authorization", token_key);

    String payload = "{\"model\": \"gpt-4\", \"temperature\": " + String(temperature) + ", \"max_tokens\": " + String(max_tokens) + ", \"messages\": [{\"role\": \"user\", \"content\": " + Question + "}]}";
    
    int httpCode = https.POST(payload);

    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String response = https.getString();
      DynamicJsonDocument doc(2048); // Increase the buffer size
      deserializeJson(doc, response);

      // Check if "choices" array exists
      if (doc.containsKey("choices")) {
        // Retrieve the first choice from the "choices" array
        String Answer = doc["choices"][0]["message"]["content"].as<String>();
        Serial.print("Answer : ");
        Serial.println(Answer);
        audio.connecttospeech(Answer.c_str(), "en");
      } else {
        Serial.println("No answer found in the response.");
      }
    } else {
      Serial.printf("[HTTPS] Request failed, error code: %d\n", httpCode);
      Serial.printf("[HTTPS] Error message: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  }
  else {
    Serial.println("[HTTPS] Unable to connect");
  }

  Question = "";
}

void audio_info(const char *info) {
  Serial.print("audio_info: ");
  Serial.println(info);
}


