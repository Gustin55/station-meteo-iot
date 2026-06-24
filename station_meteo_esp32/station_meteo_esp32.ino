#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>

// Configurations Réseau
const char* ssid = "Livebox-AFF0";
const char* password = "boulitest1806";

// Configuration Supabase
const char* supabase_url = "https://ehrcchqwechaqrsvqosa.supabase.co/rest/v1/mesures";
const char* supabase_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImVocmNjaHF3ZWNoYXFyc3Zxb3NhIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODIzMTQ5NTQsImV4cCI6MjA5Nzg5MDk1NH0.FxaBGZd5-2JWNCz8c5HyQ3YSuD35uqAW4qN2qH0dqOI";

// Configuration DHT11
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Intervalle d'envoi (ex: toutes les 60 secondes)
unsigned long previousMillis = 0;
const long interval = 60000; 

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connexion au Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnecté !");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Envoi périodique sans bloquer le microcontrôleur
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (WiFi.status() == WL_CONNECTED) {
      float h = dht.readHumidity();
      float t = dht.readTemperature();

      if (isnan(h) || isnan(t)) {
        Serial.println("Échec de lecture du DHT11");
        return;
      }

      // Création du client sécurisé pour le HTTPS
      WiFiClientSecure *client = new WiFiClientSecure;
      if(client) {
        client->setInsecure(); // Permet de se connecter en HTTPS sans stocker le certificat racine
        
        HTTPClient http;
        http.begin(*client, supabase_url);
        
        // Headers requis par l'API REST de Supabase
        http.addHeader("apikey", supabase_key);
        http.addHeader("Authorization", "Bearer " + String(supabase_key));
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Prefer", "return=minimal");

        // Construction du payload JSON
        String jsonPayload = "{\"temperature\":" + String(t) + ",\"humidity\":" + String(h) + "}";
        
        Serial.print("Envoi des données... ");
        int httpResponseCode = http.POST(jsonPayload);
        
        if (httpResponseCode > 0) {
          Serial.print("Code de réponse HTTP : ");
          Serial.println(httpResponseCode); // 201 signifie "Créé avec succès"
        } else {
          Serial.print("Erreur lors de l'envoi POST : ");
          Serial.println(http.errorToString(httpResponseCode).c_str());
        }
        
        http.end();
        delete client;
      }
    } else {
      Serial.println("Wi-Fi déconnecté !");
    }
  }
}
