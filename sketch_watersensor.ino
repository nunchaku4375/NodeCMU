/*
  Sistema de monitoreo de calidad del agua
  ----------------------------------------
  Sensores:
    - Turbidez M021.00084
    - Temperatura DS18B20

  NodeMCU ESP8266

  Conexiones Sensor de Turbidez:
    G -> GND
    V -> 3V3
    A -> A0

  Conexiones DS18B20:
    Rojo      -> 3V3
    Negro     -> GND
    Amarillo  -> D4 (GPIO2)

  IMPORTANTE:
  Colocar una resistencia de 4.7kΩ entre D4 y 3V3.

  Calibración experimental del sensor de turbidez:

    Agua limpia:
      ADC = 989

    Agua muy turbia:
      ADC = 667

  El cálculo de NTU es una aproximación
  basada en la curva del fabricante.
*/

#include <OneWire.h>
#include <DallasTemperature.h>

//-----------------------------
// Configuración DS18B20
//-----------------------------
#define PIN_DS18B20 D4

OneWire oneWire(PIN_DS18B20);
DallasTemperature sensores(&oneWire);

//-----------------------------
// Configuración Turbidez
//-----------------------------
const int TURBIDEZ_PIN = A0;

void setup()
{
  Serial.begin(115200);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" Sistema de Monitoreo de Agua");
  Serial.println(" ESP8266 + Turbidez + Temperatura");
  Serial.println("========================================");

  // Inicializar sensor de temperatura
  sensores.begin();
}

/*
  Convierte el porcentaje de transmisión
  del sensor de turbidez en una estimación
  aproximada de NTU.
*/
float estimarNTU(float porcentaje)
{
  // 0 - 500 NTU
  if (porcentaje >= 80.5)
  {
    return (100.0 - porcentaje)
         * (500.0 / (100.0 - 80.5));
  }

  // 500 - 1000 NTU
  if (porcentaje >= 64.5)
  {
    return 500.0
         + (80.5 - porcentaje)
         * (500.0 / (80.5 - 64.5));
  }

  // 1000 - 2000 NTU
  if (porcentaje >= 45.2)
  {
    return 1000.0
         + (64.5 - porcentaje)
         * (1000.0 / (64.5 - 45.2));
  }

  // 2000 - 4000 NTU
  return 2000.0
       + (45.2 - porcentaje)
       * (2000.0 / (45.2 - 22.3));
}

void loop()
{
  //--------------------------------------------------
  // Lectura del sensor de turbidez
  //--------------------------------------------------

  long suma = 0;

  // Promediar 20 muestras para reducir el ruido
  for (int i = 0; i < 20; i++)
  {
    suma += analogRead(TURBIDEZ_PIN);
    delay(10);
  }

  int adc = suma / 20;

  // Convertir la lectura a porcentaje
  float porcentaje = adc * 100.0 / 989.0;

  // Calcular NTU aproximados
  float ntu = estimarNTU(porcentaje);

  //--------------------------------------------------
  // Lectura del sensor DS18B20
  //--------------------------------------------------

  sensores.requestTemperatures();

  float temperatura = sensores.getTempCByIndex(0);

  //--------------------------------------------------
  // Mostrar resultados
  //--------------------------------------------------

  Serial.println("========================================");

  // Temperatura
  if (temperatura == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Temperatura : Sensor no encontrado");
  }
  else
  {
    Serial.print("Temperatura : ");
    Serial.print(temperatura, 2);
    Serial.println(" °C");
  }

  // Turbidez
  Serial.print("ADC         : ");
  Serial.println(adc);

  Serial.print("Porcentaje  : ");
  Serial.print(porcentaje, 1);
  Serial.println(" %");

  Serial.print("NTU aprox.  : ");
  Serial.println(ntu, 1);

  // Clasificación del agua
  Serial.print("Estado      : ");

  if (ntu < 5)
    Serial.println("Agua muy limpia");
  else if (ntu < 50)
    Serial.println("Agua ligeramente turbia");
  else if (ntu < 500)
    Serial.println("Agua moderadamente turbia");
  else if (ntu < 2000)
    Serial.println("Agua muy turbia");
  else
    Serial.println("Agua extremadamente turbia");

  Serial.println("========================================");
  Serial.println();

  delay(1000);
}