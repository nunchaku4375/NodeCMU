void setup() {
  Serial.begin(115200);
}

float estimarNTU(float porcentaje)
{
  if (porcentaje >= 80.5)
  {
    return (100.0 - porcentaje) *
           (500.0 / (100.0 - 80.5));
  }

  if (porcentaje >= 64.5)
  {
    return 500.0 +
           (80.5 - porcentaje) *
           (500.0 / (80.5 - 64.5));
  }

  if (porcentaje >= 45.2)
  {
    return 1000.0 +
           (64.5 - porcentaje) *
           (1000.0 / (64.5 - 45.2));
  }

  return 2000.0 +
         (45.2 - porcentaje) *
         (2000.0 / (45.2 - 22.3));
}

void loop()
{
  long suma = 0;

  for(int i=0;i<20;i++)
  {
    suma += analogRead(A0);
    delay(10);
  }

  int adc = suma / 20;

  float porcentaje = adc * 100.0 / 989.0;

  float ntu = estimarNTU(porcentaje);

  Serial.print("ADC: ");
  Serial.print(adc);

  Serial.print("  %: ");
  Serial.print(porcentaje,1);

  Serial.print("  NTU: ");
  Serial.println(ntu,0);

  delay(1000);
}