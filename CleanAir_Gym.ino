#include <WiFi.h>
#include <HTTPClient.h>


// Definisi pin
#define pinSen 34
#define R 15
#define G 2
#define B 4
#define buzz 5
#define relay 19

const char* ssid = "Iamd5";                        
const char* password = "kanmemang";              
const char* thingsboardUrl = "http://thingsboard.cloud/api/v1/p3hmhadlxj3z8aov8lph/telemetry"; // Token ganti punya lu
const char* telegramBotToken = "7897611044:AAGf4eVubmHK_tL98yVizh5feqMLo7itMO8";  
const char* chatID = "6082127764";              

// Kalibrasi
const float Rload = 1.0;
const float a = 110.47;
const float b = -2.862;
float Ro = 7.66, mppm = 5000;

int adc, ppm;
float Vin, Rs, ratio, Konsen;

enum AirQuality {
  BAIK,
  RATA_RATA,
  KURANG_BAIK,
  BAHAYA
};
AirQuality State;

void connectToWiFi() {
  Serial.println("Menghubungkan ke WiFi...");
  WiFi.begin(ssid, password);
  
  int attempt = 0; 
  while (WiFi.status() != WL_CONNECTED && attempt < 20) { 
    delay(500);
    Serial.print(".");
    attempt++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nTerhubung ke WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nGagal terhubung ke WiFi. Restarting...");
    ESP.restart(); 
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(pinSen, INPUT);
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(buzz, OUTPUT);
  pinMode(relay, OUTPUT);

  digitalWrite(R, LOW);
  digitalWrite(G, LOW);
  digitalWrite(B, LOW);
  digitalWrite(buzz, LOW);
  digitalWrite(relay, LOW);

  connectToWiFi(); 
}

void loop() {
  adc = analogRead(pinSen);
  Vin = (adc / 4095.0) * 3.3;
  Rs = ((3.3 - Vin) / Vin) * Rload;
  ratio = Rs / Ro;
  ppm = (a * pow(ratio, b) / 10) + 400;
  Konsen = (ppm / mppm) * 100;

  Serial.println("-------------------");
  Serial.print("| PPM : ");
  Serial.print(ppm);
  Serial.print(" ppm | CO2 : ");
  Serial.print(Konsen);
  Serial.println(" % |");

  updateState(); 
  control();     
  sendToThingsBoard(ppm, Konsen); 

  delay(2000);
}

void sendToThingsBoard(int ppm, float konsen) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(thingsboardUrl);
    http.addHeader("Content-Type", "application/json");

    String payload = "{";
    payload += "\"ppm\":" + String(ppm) + ",";
    payload += "\"CO2\":" + String(konsen);
    payload += "}";

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      Serial.print("Data terkirim: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Gagal mengirim data, kode error: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi tidak terhubung. Tidak bisa mengirim data.");
  }
}

void sendMessageToTelegram(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String("https://api.telegram.org/bot") + telegramBotToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;
    http.begin(url);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.println("Notifikasi terkirim ke Telegram.");
    } else {
      Serial.println("Gagal mengirim notifikasi Telegram.");
    }
    http.end();
  } else {
    Serial.println("WiFi tidak terhubung. Tidak bisa mengirim notifikasi Telegram.");
  }
}

void setRGB(int r, int g, int b) {
  digitalWrite(R, r);
  digitalWrite(G, g);
  digitalWrite(B, b);
}

void updateState() {
  Serial.print("Kualitas Udara : ");
  if (ppm >= 400 && ppm <= 1000) {
    State = BAIK;
    Serial.println("Baik");
  } else if (ppm > 1000 && ppm <= 1300) {
    State = RATA_RATA;
    Serial.println("Rata-Rata");
  } else if (ppm > 1300 && ppm <= 2000) {
    State = KURANG_BAIK;
    Serial.println("Kurang Baik");
  } else if (ppm > 2000) {
    State = BAHAYA;
    Serial.println("Bahaya");
    Serial.println("Evakuasi Segera..!");
  }
}

void control() {
  String message;
  switch (State) {
    case BAIK:
      setRGB(0, 0, 1); 
      message = "Kualitas Udara Baik 😊";
      break;
    case RATA_RATA:
      setRGB(0, 1, 0); 
      message = "Kualitas Udara Rata-Rata 😐";
      break;
    case KURANG_BAIK:
      setRGB(1, 1, 0); 
      message = "Kualitas Udara Kurang Baik 😷";
      break;
    case BAHAYA:
      setRGB(1, 0, 0);
      tone(buzz, 3000, 200);
      digitalWrite(relay, HIGH);
      message = "Kualitas Udara Bahaya! Evakuasi Segera 😱";
      break;
  }
  sendMessageToTelegram(message);
}
