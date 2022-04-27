#define BLYNK_TEMPLATE_ID "TMPLpovunLGD"
#define BLYNK_DEVICE_NAME "EUII"
#define BLYNK_AUTH_TOKEN "GryGHjnTKGsUMBPazVgQaXyB0n1eH2CL"

#include <ESPDateTime.h> //Biblioteca para converter Horas
#include <ESP8266WiFi.h> //Biblioteca para usar o Wi-Fi no Esp8266
#include <ESP8266WebServer.h> // Biblioteca para inicializar o servidor na placa
#include <ESP8266HTTPClient.h> //Biblioteca para realizar chamadas HTTP
#include <WiFiClient.h>
#include <ArduinoJson.h> //Biblioteca processamento de JSON
#include <BlynkSimpleEsp8266.h> //Biblioteca do aplicativo Blynk
#include "DHT.h" //Biblioteca do sensor de umidade e temperatura


#define DHTPIN 2 // Pino ao qual o sensor foi conectado no Esp (D4)
#define DHTTYPE DHT11 //pode substituir o "DHT11" pelo "DHT22"

#define HTTP_REST_PORT 8080 // porta do servidor rest
ESP8266WebServer httpRestServer(HTTP_REST_PORT); // definindo o server


//Setando constantes de funcionamento
const char* serverAddress = "http://automation-eng-unificada.herokuapp.com/api/automation/updateReadings";
const char auth[] = BLYNK_AUTH_TOKEN;//inserir código de autenticação enviado por e-mail pelo app blynk
const char ssid[] = "";//inserir nome da rede Wi-Fi
const char pass[] = "";//inserir senha da rede wi-fi
char jsonString[256];
BlynkTimer timer;
BlynkTimer serverUpdateTimer;
DHT dht(DHTPIN, DHTTYPE);

//Variaveis Sensor temperatura/Umidade
float humidity = 0;
float temperature = 0;

//Variaveis SENSOR pH
#define SensorPin 0                                                    //A SAÍDA ANALÓGICA DO MEDIDOR DE PH ESTÁ CONECTADA COM O ANALÓGICO DA PLACA
long int phMedio;                                                      //VALOR MÉDIO DO SENSOR
float b;
int buf[10], temp;

#define PIN_BUZZER D8;                                                 // ADICIONAR BUZZER PARA APONTAR FINAL DAS 72 HORAS

//PARÂMETROS DEFAULT:

//UMIDADE
float maxUmid = 55;     //%
float minUmid = 45;     //%

//TEMPERATURA
float maxTemp = 65;
float minTemp = 60;

//Controle de processamento
int isProcessing = 0;
time_t processingStartTime = 0;
time_t processingEndTime = 72;
int currentState = 0;
int prevState = -1;


void updateBlynk()
{
  updateDataSensor();
  Blynk.virtualWrite(V5, humidity); //O leitor de umidade do Blynk tem que estar nesse pino virtual
  Blynk.virtualWrite(V6, temperature); //O leitor de temperatura do Blynk tem que estar nesse pino virtual
}

void sendUpdateRequest()                                                 // ENVIAR OS VALORES PARA O SERVIDOR
{

  float currentTemperature = getTemperature();
  float currentHumidity = getHumidity();
  float currentPH = getPh();

  DynamicJsonDocument doc(1024);
  JsonObject payload  = doc.createNestedObject("payload");
  payload["temp"] = currentTemperature;
  payload["humidity"] = currentHumidity;
  payload["pH"] = currentPH;

  serializeJson(doc, jsonString);

  Serial.println((String)"Payload => " + jsonString);


  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, serverAddress);

    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonString);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode == 201) {
      Serial.println((String)"Banco de dados atualizado com sucesso");
    }
    // Free resources
    http.end();

  }
}

void setup()
{
  //Setando bit rate da comunicacao serial
  Serial.begin(115200);

  //Inicializando sensor de temperatura
  dht.begin();

  //Iniciando conexao com blynk
  Blynk.begin(auth, ssid, pass);

  //Iniciando conexao wifi
  establishConn();

  //Setando eventos triggados por tempo
  timer.setInterval(2000L, updateBlynk);
  serverUpdateTimer.setInterval(5 * 60 * 1000, sendUpdateRequest);

  //Inicializando servidor REST
  restServerRouting();
  httpRestServer.begin();

}

void establishConn() {
  WiFi.begin(ssid, pass);
  Serial.println("Connecting to wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

//Controller das rotas
void getSensorData() {
  Serial.println("Recuperando leitura dos sensores");
  float currentTemperature = getTemperature();
  float currentHumidity = getHumidity();
  float currentPH = getPh();
  
  DynamicJsonDocument doc(1024);
  JsonObject payload  = doc.createNestedObject("data");
  payload["temp"] = currentTemperature;
  payload["humidity"] = currentHumidity;
  payload["pH"] = currentPH;
  char returnString[1024];

  serializeJson(doc, returnString);

  Serial.println("Retornando leitura dos sensores");
  httpRestServer.send(200, F("application/json"), returnString);
}

void getSystemStatus() {
  Serial.println("Recuperando Status do Sistema");
  DynamicJsonDocument doc(1024);
  JsonObject payload  = doc.createNestedObject("data");
  payload["minUmid"] = minUmid;
  payload["maxUmid"] = maxUmid;
  payload["minTemp"] = minTemp;
  payload["maxTemp"] = maxTemp;
  if(isProcessing){
    payload["isProcessing"] = true;
  } else {
    payload["isProcessing"] = false;
  }

  char returnString[1024];

  serializeJson(doc, returnString);

  Serial.println("Enviando status do Sistema");
  httpRestServer.send(200, F("application/json"), returnString);
}

void postRemoteActions() {
    String postBody = httpRestServer.arg("plain");
    Serial.println("Recebido ==> \n" + postBody + "\n");
 
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, postBody);
    if (error) {
        Serial.print(F("Error parsing JSON "));
        Serial.println(error.c_str());
 
        String msg = error.c_str();
 
        httpRestServer.send(400, F("text/html"),"Error ao dar parse no json recebido! <br>" + msg);
 
    } else {
        JsonObject postObj = doc.as<JsonObject>();
 
        if (httpRestServer.method() == HTTP_POST) {
            if (postObj.containsKey("subFunction")) {
 
                String subRotina = postObj["subFunction"];

                if(subRotina == "ligarMotor"){
                  ligarMotor();
                } else if (subRotina == "desligarMotor"){
                  desligarMotor();
                } else if (subRotina == "desligarTorneira"){
                  desligarTorneira();
                } else if (subRotina == "ligarTorneira") {
                  ligarTorneira();
                } else if (subRotina == "iniciarProcessamento"){
                  iniciarProcessamento();
                } else if (subRotina == "desligarProcessamento"){
                  desligarProcessamento();
                } else {
                  DynamicJsonDocument doc(512);
                  doc["isSucess"] = false;
                  doc["message"]= "Subrotina inválida!";
  
                  String buf;
                  serializeJson(doc, buf);
  
                  httpRestServer.send(422, F("application/json"), buf);
                }
                
                DynamicJsonDocument doc(512);
                doc["isSucess"] = true;
                doc["message"]= "Subrotina acionada com sucesso!";
 
                String buf;
                serializeJson(doc, buf);
 
                httpRestServer.send(201, F("application/json"), buf);
            }else {
                DynamicJsonDocument doc(512);
                doc["isSucess"] = false;
                doc["message"] = "Body Inválido";
 
                String buf;
                serializeJson(doc, buf);
 
                httpRestServer.send(404, F("application/json"), buf);
            }
        }
    }
}

//Rotas da API
void restServerRouting() {
    httpRestServer.on(F("/sensorData"), HTTP_GET, getSensorData);
    httpRestServer.on(F("/systemStatus"), HTTP_GET, getSystemStatus);
    httpRestServer.on(F("/remoteActions"), HTTP_POST, postRemoteActions);
}

void loop()
{
  delay(1000);
  Blynk.run();
  timer.run();
  serverUpdateTimer.run();
  httpRestServer.handleClient();


  // Variavel que seta se a composteira esta ligada ou nao. Acionada via Blynk ou Acao remota
  if (isProcessing)
  {
    time_t now = DateTime.now();
    int elapsed = now - processingStartTime;
    currentState = getCurrentState(currentState);

    if (currentState != prevState) {
      Serial.println((String)"Dados atuais:\n Umidade: " + humidity + "\n Temperatura: " + temperature);
      Serial.println();
      Serial.println((String) "Dados de controle atuais:\n Umidade - Máx = " + maxUmid + " Min = " + minUmid);
      Serial.println((String) " Temperatura - Máx = " + maxTemp + " Min = " + minTemp);
      Serial.println();
      
      switch (currentState) {
        case 1:
          Serial.println("Entrando no estado de inicialização");
          ligarMotor();
          break;

        case 2:
          // "LOW-TEMP LOW-HUM"
          Serial.println("Umidade e Temperatura abaixo dos mínimo.");
          ligarAquecedor();
          ligarTorneira();
          break;

        case 3:
          //"LOW-TEMP HIGH-HUM"
          Serial.println("Temperatura abaixo dos mínimo, umidade acima do máximo.");
          ligarAquecedor();
          desligarTorneira();
          break;

        case 4 :
          //"HIGH-TEMP LOW-HUM"
          Serial.println("Umidade abaixo do mínimo, temperatura acima do máximo");
          // Teoricamente ja esta desligado, forcando o desligamento para prevenir.
          desligarAquecedor();
          ligarTorneira();
          break;

        case 5:
          //"HIGH-TEMP HIGH-HUM"
          Serial.println("Umidade e temperatura acima do máximo.");
          // Teoricamente ja estao desligados, forcando o desligamento para prevenir.
          desligarAquecedor();
          desligarTorneira();
          break;

        case 6:
          Serial.println("Temperatura abaixo do mínimo.");
          // Teoricamente ja estao desligados, forcando o desligamento para prevenir.
          ligarAquecedor();
          break;

        case 7:
          Serial.println("Temperatura acima do máximo.");
          desligarAquecedor();
          break;

        case 8:
          Serial.println("Umidade abaixo do mínimo.");
          ligarTorneira();
          break;

        case 9:
          Serial.println("Umidade acima do máximo.");
          desligarTorneira();
          break;

      }
      prevState = currentState;
    }


    if (elapsed -  processingEndTime * 3600 > 0) {
      isProcessing = false;

      Serial.println("Fim de processamento. Desligando subrotinas...");
      desligarMotor();
      desligarTorneira();
      desligarAquecedor();
      desligarTorneira();

    }
  }
}

int getCurrentState(int state) {
  if (getTemperature() < minTemp) {
    if (getHumidity() < minUmid) {
      return 2;
    } else if (getHumidity() > maxUmid) {
      return 3;
    } else {
      return 6;
    }
  } else if (getTemperature() > maxTemp) {
    if (getHumidity() < minUmid) {
      return 4;
    } else if (getHumidity() > maxUmid) {
      return 5;
    } else {
      return 7;
    }
  } else if (getHumidity() < minUmid) {
    if (getTemperature() < minTemp) {
      return 2;
    } else if (getTemperature() > maxTemp) {
      return 4;
    } else {
      return 8;
    }
  } else if (getHumidity() > maxUmid) {
    if (getTemperature() < minTemp) {
      return 3;
    } else if (getTemperature() > maxTemp) {
      return 5;
    } else {
      return 9;
    }
  } else {
    return state;
  }
}

void updateDataSensor()
{
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}

float getTemperature()
{
  return dht.readTemperature();
}

float getHumidity()
{
  return dht.readHumidity();
}

float getPh()
{ //SENSOR pH
  for (int i = 0; i < 10; i++)                                          //OBTER 10 VALORES PARA MÉDIA
  {
    buf[i] = analogRead(SensorPin);
    delay(10);
  }
  for (int i = 0; i < 9; i++)                                           //CLASSIFICAR DE PEQUENO PARA GRANDE
  {
    for (int j = i + 1; j < 10; j++)
    {
      if (buf[i] > buf[j])
      {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }

  phMedio = 0;
  for (int i = 2; i < 8; i++)                                          // EXCLUINDO AS 2 MENORES E 2 MAIORES, PEGAR A MÉDIA DAS 6 RESTANTES
    phMedio += buf[i];
  float phValor = (float)phMedio * 5.0 / 1024 / 6;                     //CONVERSÃO EM MILIVOLT
  phValor = 3.5 * phValor;                                             //CONVERSÃO DE MILIVOLT PARA pH

  if (!phValor) {
    phValor = 0;
  }

  return phValor;
}

void desligarTorneira()
{
  digitalWrite(D5, LOW);
  delay(10);
  Serial.println((String) "Desligando a torneira");
}

void ligarTorneira()
{
  digitalWrite(D5, HIGH);
  delay(10);
  Serial.println((String) "Ligando a torneira");
}
void ligarMotor()
{
  digitalWrite(D2, HIGH);
  delay(10);
  Serial.println((String) "Ligando o motor");
}
void desligarMotor()
{
  digitalWrite(D2, LOW);
  delay(10);
  Serial.println((String) "Desligando o motor");
}
// CONTROLE DO AQUECEDOR
void ligarAquecedor()                                                     // LIGAR AQUECEDOR
{
  digitalWrite(D2, HIGH);                                                 // COLOCAR O PINO CORRETO
  delay(10);
  Serial.println((String) "Ligando o aquecedor");
}
void desligarAquecedor()                                                  // DESLIGAR AQUECEDOR
{
  digitalWrite(D2, LOW);                                                  // COLOCAR O PINO CORRETO
  delay(10);
  Serial.println((String) "desligando o aquecedor");
}

void iniciarProcessamento(){
  isProcessing = 1;
  Serial.println((String) "Iniciando processamento por ação remota");  
}

void desligarProcessamento(){
  isProcessing = 0;
  Serial.println((String) "Finalizando processamento por ação remota");  
}


//CALLBACKS PINOS VIRTUAIS
BLYNK_WRITE(V3)
{
  minUmid = param.asFloat(); // Get value as integer
  Serial.println((String) "Nova umidade mínima definida " + minUmid + "%");             // PALAVRAS ALTERADAS
}

BLYNK_WRITE(V4)
{
  maxUmid = param.asFloat(); // Get value as integer
  Serial.println((String) "Nova umidade máxima definida " + maxUmid + "%");             // PALAVRAS ALTERADAS
}

BLYNK_WRITE(V8)
{
  isProcessing = param.asInt(); // Get value as integer
  processingStartTime = DateTime.now();
  Serial.println((String) "Iniciando processamento...");                // PALAVRAS ALTERADAS
}