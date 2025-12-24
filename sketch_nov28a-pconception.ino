#include <WiFi.h>
#include <PubSubClient.h>

// ===============================
// CONFIG WIFI
// ===============================
const char* ssid = "AndroidAP5ACF";
const char* password = "azerty1234";

// ===============================
// THINGSBOARD
// ===============================
const char* mqtt_server = "demo.thingsboard.io";
const int mqtt_port = 1883;
const char* access_token = "zcf4g3x0pmb5pmicyc5s";

// ===============================
WiFiClient espClient;
PubSubClient client(espClient);

// ===============================
// VARIABLES SIMULÉES
// ===============================
float voltage = 220.0;
float fridge  = 1.0;
float ac      = 3.8;
float tv      = 0.7;
float lampe   = 0.2;

// ===============================
// WIFI
// ===============================
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("📡 WiFi");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 20) {
    delay(500);
    Serial.print(".");
    i++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connecté");
  } else {
    Serial.println("\n❌ WiFi échec");
  }
}

// ===============================
// MQTT
// ===============================
void reconnectMQTT() {
  while (!client.connected()) {
    String clientId = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    if (client.connect(clientId.c_str(), access_token, NULL)) {
      Serial.println("✅ MQTT connecté");
    } else {
      Serial.print("❌ MQTT erreur ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// ===============================
// SETUP
// ===============================
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));

  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(1024);   // 🔴 TRÈS IMPORTANT
}

// ===============================
// LOOP
// ===============================
void loop() {

  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!client.connected()) reconnectMQTT();
  client.loop();

  // ===============================
  // VARIATION DES VALEURS
  // ===============================
  fridge  += random(-5,6)/100.0;
  ac      += random(-5,6)/100.0;
  tv      += random(-5,6)/100.0;
  lampe   += random(-5,6)/100.0;
  voltage += random(-3,4);

  fridge  = constrain(fridge, 0.8, 1.6);
  ac      = constrain(ac, 3.5, 4.5);
  tv      = constrain(tv, 0.5, 1.2);
  lampe   = constrain(lampe, 0.0, 0.6);
  voltage = constrain(voltage, 200, 240);

  float total_conso = fridge + ac + tv + lampe;

  // ===============================
  // ALERTES SIMPLES
  // ===============================
  String alertes = "";

  if (fridge > 1.5) alertes += "FRIGO_CRITICAL|";
  else if (fridge > 1.3) alertes += "FRIGO_WARNING|";
  else alertes += "FRIGO_OK|";

  if (ac > 4.3) alertes += "CLIM_CRITICAL|";
  else if (ac > 4.0) alertes += "CLIM_WARNING|";
  else alertes += "CLIM_OK|";

  if (tv > 1.1) alertes += "TV_CRITICAL|";
  else if (tv > 0.9) alertes += "TV_WARNING|";
  else alertes += "TV_OK|";

  if (lampe > 0.5) alertes += "LAMPE_CRITICAL|";
  else if (lampe > 0.3) alertes += "LAMPE_WARNING|";
  else alertes += "LAMPE_OK|";

  if (voltage < 205 || voltage > 235) alertes += "TENSION_CRITICAL|";
  else alertes += "TENSION_OK|";

  if (total_conso > 5) alertes += "TOTAL_CRITICAL";
  else if (total_conso > 4) alertes += "TOTAL_WARNING";
  else alertes += "TOTAL_OK";

  // ===============================
  // PAYLOAD JSON
  // ===============================
  String payload = "{";
  payload += "\"tension\":" + String(voltage,1) + ",";
  payload += "\"frigo\":" + String(fridge,2) + ",";
  payload += "\"clim\":" + String(ac,2) + ",";
  payload += "\"tv\":" + String(tv,2) + ",";
  payload += "\"lampe\":" + String(lampe,2) + ",";
  payload += "\"total_conso\":" + String(total_conso,2) + ",";
  payload += "\"alertes\":\"" + alertes + "\"";
  payload += "}";

  Serial.println("📤 Envoi telemetry");
  Serial.println(payload);

  client.publish("v1/devices/me/telemetry", payload.c_str());

  delay(2000);
}
