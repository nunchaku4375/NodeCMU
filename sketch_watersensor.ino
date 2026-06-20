/*
  Sensor de Turbidez M021.00084
  NodeMCU ESP8266

  Conexiones:
  G -> GND
  V -> 3V3
  A -> A0

  Calibración experimental obtenida:

  Agua limpia:
  ADC = 989

  Agua muy turbia:
  ADC = 667

  El cálculo de NTU es una aproximación
  basada en la curva del fabricante.
*/

void setup()
{
  Serial.begin(115200);

  Serial.println();
  Serial.println("Sistema de medicion de turbidez");
}

/*
  Convierte el porcentaje de transmisión
  en una estimación de NTU utilizando
  interpolación por segmentos.
*/
float estimarNTU(float porcentaje)
{
  // Zona entre 0 y 500 NTU
  if (porcentaje >= 80.5)
  {
    return (100.0 - porcentaje)
         * (500.0 / (100.0 - 80.5));
  }

  // Zona entre 500 y 1000 NTU
  if (porcentaje >= 64.5)
  {
    return 500.0
         + (80.5 - porcentaje)
         * (500.0 / (80.5 - 64.5));
  }

  // Zona entre 1000 y 2000 NTU
  if (porcentaje >= 45.2)
  {
    return 1000.0
         + (64.5 - porcentaje)
         * (1000.0 / (64.5 - 45.2));
  }

  // Zona entre 2000 y 4000 NTU
  return 2000.0
       + (45.2 - porcentaje)
       * (2000.0 / (45.2 - 22.3));
}

void loop()
{
  /*
    Promedio de 20 muestras
    para reducir ruido eléctrico.
  */
  long suma = 0;

  for(int i = 0; i < 20; i++)
  {
    suma += analogRead(A0);

    delay(10);
  }

  int adc = suma / 20;

  /*
    Conversión a porcentaje relativo.

    989 ADC representa
    agua extremadamente limpia.
  */
  float porcentaje = adc * 100.0 / 989.0;

  /*
    Conversión del porcentaje
    a NTU aproximados.
  */
  float ntu = estimarNTU(porcentaje);

  Serial.print("ADC: ");
  Serial.print(adc);

  Serial.print("  Porcentaje: ");
  Serial.print(porcentaje, 1);
  Serial.print("%");

  Serial.print("  NTU: ");
  Serial.println(ntu, 0);

  delay(1000);
}