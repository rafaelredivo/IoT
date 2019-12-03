//Programa: Monitoramento de reservatório com ESP32 Wifi e protocolo MQTT

//mqtt
#include <WiFi.h>
#include <PubSubClient.h>
#include <CESmartCamp.h>

//sensor flow
#define LED_BUILTIN 5
#define SENSOR  2

//sensor ultrasonic
#define TRIGGER_PIN 4
#define ECHO_PIN 5
#define pi 3.1415927
#define circleRadius 0.039 //[m]
#define height 0.096 //[m]
UltraSonic sonar(TRIGGER_PIN, ECHO_PIN);

//mqtt variaveis
const char* ssid = "RedmiRedivo";
const char* password =  "r1234567";
const char* mqttServer = "tailor.cloudmqtt.com";
const int mqttPort = 15533;
const char* mqttUser = "appxwgdv";
const char* mqttPassword = "9LdxDT8OpGW3";

//sensor variaveis
long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
int reservatorio = 100000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long totalLitre = 0;
unsigned long totalCubic;
unsigned long reservatorioAtual;

float metres = 0, areaCircle, volume, waterTankLitres;
unsigned long centimetres = 0 ;

WiFiClient espClient;
PubSubClient client(espClient);

//publish
int contador = 1;
char mensagem[30];
char mensagem2[30];
char mensagem3[30];


void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}


void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Iniciando conexao com a rede WiFi..");
  }

  Serial.println("Conectado na rede WiFi!");

  pinMode(SENSOR, INPUT_PULLUP);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);
}


void loop()
{
   //calling the ultrasonic sensor function here
   ultrasonicSensor();

  //calling the water flow sensor function here
   waterFlowSensor();

   //calling the publish function here
   publishFunction();
  

}

 
void publishFunction()
{
  //Faz a conexao com o broker MQTT
  reconectabroker();
  sprintf(mensagem, "%ld", totalMilliLitres);
  Serial.print("Mensagem enviada: ");
  Serial.println(mensagem);

  //Envia a mensagem ao broker
  client.publish("MQTTRafael", mensagem);

  sprintf(mensagem2, "%ld", totalLitre);
  Serial.print("Mensagem enviada: ");
  Serial.println(mensagem2);

  //Envia a mensagem2 ao broker
  client.publish("MQTTRafael2", mensagem2);

  sprintf(mensagem3, "%f", waterTankLitres);
  //Envia a mensagem3 ao broker
  client.publish("MQTTRafael3", mensagem3);
  delay(1000);
}


void waterFlowSensor()
{
  currentMillis = millis();
  if (currentMillis - previousMillis > interval) {

    pulse1Sec = pulseCount;
    pulseCount = 0;

    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
    totalLitre = totalMilliLitres / 1000;
    totalCubic = totalLitre / 1000;

    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t");       // Print tab space

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    Serial.print("mL / ");
    Serial.print(totalLitre,4);
    Serial.println("L / ");
    Serial.print(totalCubic);
    Serial.println("m3");

    //calculos extras
    reservatorioAtual = reservatorio - totalMilliLitres;
    Serial.print("Nível atual: ");
    Serial.print(reservatorioAtual);
    Serial.println("ml");
    delay(1000);
  }
}


void ultrasonicSensor()
{
  float(centimetres) = sonar.length_cm();
  float(metres) = centimetres / 100;

  areaCircle = pi * circleRadius * circleRadius;
  volume = areaCircle * (height - metres);

  waterTankLitres = volume * 1000;

  Serial.print(" ");
  Serial.print(centimetres);
  Serial.print("cm");
  Serial.print(" ");
  Serial.println(sonar.length_inch());
  Serial.print(" ");
  Serial.print(metres, 4);
  Serial.print("m");
  Serial.print("Area do circulo: ");
  Serial.println(areaCircle, 4);
  Serial.print("Volume: ");
  Serial.println(volume, 4);
  Serial.print("m³");
  Serial.print("Reservatorio Atual: ");
  Serial.println(waterTankLitres, 4);
  Serial.print("L");

  delay(1000);
}


void reconectabroker()
{
  //Conexao ao broker MQTT
  client.setServer(mqttServer, mqttPort);
  while (!client.connected())
  {
    Serial.println("Conectando ao broker MQTT...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword ))
    {
      Serial.println("Conectado ao broker!");
    }
    else
    {
      Serial.print("Falha na conexao ao broker - Estado: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}
