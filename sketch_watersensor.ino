#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//--------------------------------------------------
// Configuración de Red (Zona Wifi de su celular)
//--------------------------------------------------
const char* ssid = "Nunchaku-iPhone";
const char* password = "jaco1234";

// Dirección IP de la Raspberry Pi (Es más seguro usar la IP que raspberry10.local)
const char* mqtt_server = "172.20.10.7"; // <--- Reemplace por la IP de su Raspberry Pi
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

//--------------------------------------------------
// Configuración Sensores
//--------------------------------------------------
// Temperatura DS18B20
#define PIN_DS18B20 4
OneWire oneWire(PIN_DS18B20);
DallasTemperature sensores(&oneWire);

// Turbidez (GPIO 32 es ADC1)
const int TURBIDEZ_PIN = 32;

// pH (GPIO 33 es ADC1)
const int PH_PIN = 33;

// Variables de calibración de pH (ajustar experimentalmente)
float ph_offset = 0.00; // Ajuste para calibración fina

void setup() {
  Serial.begin(115200);
  
  // Forzar resolución de 10 bits (0-1023) para mantener calibración previa del ESP8266
  analogReadResolution(10); 

  sensores.begin();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("Dirección IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Bucle hasta lograr la conexión al broker MQTT
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    // Intentar conectar con un ID único
    String clientId = "ESP32_Monitoreo_Agua_";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado al broker MQTT");
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

// Función de estimación de NTU (su algoritmo original)
float estimarNTU(float porcentaje) {
  if (porcentaje >= 80.5) return (100.0 - porcentaje) * (500.0 / (100.0 - 80.5));
  if (porcentaje >= 64.5) return 500.0 + (80.5 - porcentaje) * (500.0 / (80.5 - 64.5));
  if (porcentaje >= 45.2) return 1000.0 + (64.5 - porcentaje) * (1000.0 / (64.5 - 45.2));
  return 2000.0 + (45.2 - porcentaje) * (2000.0 / (45.2 - 22.3));
}

// Función para calcular el pH a partir del voltaje del sensor
float leerPH() {
  long suma = 0;
  for (int i = 0; i < 20; i++) {
    suma += analogRead(PH_PIN);
    delay(10);
  }
  float adc = suma / 20.0;
  
  // Convertir lectura analógica a voltaje en el ESP32 (0 - 3.3V)
  float voltaje = adc * (3.3 / 1023.0);
  
  // Ecuación lineal típica para el PH-4502C: 
  // pH 7 suele entregar aproximadamente 2.5V (ajustable con el potenciómetro de la placa).
  // La sensibilidad estándar es de aprox -5.7mV/pH.
  float valorPH = 3.5 * voltaje + ph_offset; 
  
  return valorPH;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //--------------------------------------------------
  // 1. Lectura Turbidez
  //--------------------------------------------------
  long sumaTurbidez = 0;
  for (int i = 0; i < 20; i++) {
    sumaTurbidez += analogRead(TURBIDEZ_PIN);
    delay(10);
  }
  int adc_turbidez = sumaTurbidez / 20;
  float porcentaje_turbidez = adc_turbidez * 100.0 / 1017;
  float ntu = estimarNTU(porcentaje_turbidez);

  //--------------------------------------------------
  // 2. Lectura Temperatura
  //--------------------------------------------------
  sensores.requestTemperatures();
  float temperatura = sensores.getTempCByIndex(0);

  //--------------------------------------------------
  // 3. Lectura pH
  //--------------------------------------------------
  float pH = leerPH();

  //--------------------------------------------------
  // 4. Mostrar en Serial
  //--------------------------------------------------
  Serial.println("========================================");
  Serial.print("Temp: "); Serial.print(temperatura); Serial.println(" C");
  Serial.print("Turbidez NTU: "); Serial.println(ntu);
  Serial.print("pH: "); Serial.println(pH);
  Serial.println("========================================");

  //--------------------------------------------------
  // 5. Enviar por MQTT en formato JSON
  //--------------------------------------------------
  // Construcción del string en formato JSON de forma manual
  char payload[150];
  snprintf(payload, sizeof(payload), 
           "{\"temperatura\":%.2f,\"turbidez\":%.1f,\"pH\":%.2f}", 
           temperatura, ntu, pH);

  Serial.print("Publicando payload: ");
  Serial.println(payload);

  // Publicar en el tema "agua/sensores"
  client.publish("agua/sensores", payload);

  // Esperar 5 segundos antes de la siguiente lectura
  delay(5000);
}