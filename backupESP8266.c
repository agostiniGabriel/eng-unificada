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
ESP8266WebServer httpRestServer(HTTP_REST_PORT); // definindo o servidor


//Setando constantes de funcionamento e logando no app blynk
const char* serverAddress = "http://automation-eng-unificada.herokuapp.com/api/automation/updateReadings"; //endereco do servidor intermediário
const char auth[] = BLYNK_AUTH_TOKEN;//inserir código de autenticação enviado por e-mail pelo app blynk ou na página de configuração do blynk
const char ssid[] = "";//inserir nome da rede Wi-Fi
const char pass[] = "";//inserir senha da rede wi-fi
char jsonString[256];
BlynkTimer timer; //declarando timer para atualizar o blynk
BlynkTimer serverUpdateTimer;//declarando timer para atualizar banco de dados da equipe de dados
DHT dht(DHTPIN, DHTTYPE); //setando o sensor te temperatura, usando a porta e o tipo de sensor

//DEFINIÇÃO DE VARIÁVEIS
//Sensor temperatura/Umidade
float humidity = 0;
float temperature = 0;

//SENSOR pH
#define SensorPin 0 //A saída analógica do medidor de pH está conectada com o analógico da placa
long int phMedio;   //Valor médio do sensor pH
float b;
int buf[10], temp;

//PARAMETROS DEFAULT:
//Se não forem alterados o sistema funcionara conforme estes parâmetros

//Umidade
float maxUmid = 55;
float minUmid = 45;

//Temperatura
float maxTemp = 65;
float minTemp = 60;

//CONTROLE DO PROCESSAMENTO
int isProcessing = 0;
time_t processingStartTime = 0;
time_t processingEndTime = 72;

//Estados de controle, qual a situação atual e qual a anterior
int currentState = 0;
int prevState = -1;

//Funcao que ira mandar os dados para o blynk de x em x segundos. X é definido no setup()
void updateBlynk()
{
  //atualizamos as variaveis de temperatura e umidade
  updateDataSensor();

  //agora mandamos para o blynk, essas portas V5 e V6 são o iD do item do layout do blynk.
  Blynk.virtualWrite(V5, humidity); //O leitor de umidade do Blynk tem que estar nesse pino virtual
  Blynk.virtualWrite(V6, temperature); //O leitor de temperatura do Blynk tem que estar nesse pino virtual
}

//Enviar valores para servidor intermediário atualizar o banco de dados. Executado de x em x segundos. X é definido no setup()
void sendUpdateRequest()
{

//Leitura dos sensores
  float currentTemperature = getTemperature();
  float currentHumidity = getHumidity();
  float currentPH = getPh();

  //Montagem do objeto JSON para envio
  DynamicJsonDocument doc(1024);
  JsonObject payload  = doc.createNestedObject("payload");
  payload["temp"] = currentTemperature;
  payload["humidity"] = currentHumidity;
  payload["pH"] = currentPH;

  //Serializando o Json
  serializeJson(doc, jsonString);

  Serial.println((String)"Payload => " + jsonString);

  //Verificação da conectividade para envio do objeto
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    //Iniciando a requisição
    http.begin(client, serverAddress);

    //Adicionando os headers da chamada
    http.addHeader("Content-Type", "application/json");
  
    //Realizando o envio e pegando o status code
    int httpResponseCode = http.POST(jsonString);

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    //Validando se o envio foi realizado com sucesso
    if (httpResponseCode == 201) {
      Serial.println((String)"Banco de dados atualizado com sucesso");
    }
    http.end();
  }
}

//Inicializando componentes, timers, servidor rest
void setup()
{
  //Setando bit rate da comunicação serial
  Serial.begin(115200);

  //Inicializando sensor de temperatura
  dht.begin();

  //Iniciando conexao com blynk
  Blynk.begin(auth, ssid, pass);

  //Iniciando conexao wifi
  establishConn();

  //Setando eventos triggados (disparados) por tempo. 
  //Atualização do blynk de 2 segundos em 2 segundos
  timer.setInterval(2000L, updateBlynk);

  //Setando a atualizacao do banco de dados de 5 em 5 minutos
  serverUpdateTimer.setInterval(5 * 60 * 1000, sendUpdateRequest);

  //Inicializando servidor REST. Vai receber as ações remotas
  restServerRouting();
  httpRestServer.begin();

}

//Logando e estabelecendo a conexão com a internet
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
//Este Controller define oq deve ser retornado quando alguem executa a ação remota GET /sensorData
void getSensorData() {
  //Pegando as informações dos sensores
  Serial.println("Recuperando leitura dos sensores");
  float currentTemperature = getTemperature();
  float currentHumidity = getHumidity();
  float currentPH = getPh();

  //Criando objeto para retornar para quem executou a ação remota
  DynamicJsonDocument doc(1024);
  JsonObject payload  = doc.createNestedObject("data");
  payload["temp"] = currentTemperature;
  payload["humidity"] = currentHumidity;
  payload["pH"] = currentPH;
  char returnString[1024];

  serializeJson(doc, returnString);

  //Mandando as leituras para quem solicitou
  Serial.println("Retornando leitura dos sensores");
  httpRestServer.send(200, F("application/json"), returnString);
}

//Esta rota define oq deve ser retornado quando alguem executa a ação remota GET /systemStatus
//Este Controller retorna quais os parâmetros atuais do sistema, incluindo se a composteira está ou não ligada
void getSystemStatus() {
  Serial.println("Recuperando Status do Sistema");
  DynamicJsonDocument doc(1024);
  
  //Pegando as informações do sistema
  JsonObject payload  = doc.createNestedObject("data");
  payload["minUmid"] = minUmid;
  payload["maxUmid"] = maxUmid;
  payload["minTemp"] = minTemp;
  payload["maxTemp"] = maxTemp;
  if (isProcessing) {
    payload["isProcessing"] = true;
  } else {
    payload["isProcessing"] = false;
  }

  char returnString[1024];

  serializeJson(doc, returnString);

  //Mandando as leituras para quem solicitou
  Serial.println("Enviando status do Sistema");
  httpRestServer.send(200, F("application/json"), returnString);
}

//Controller que executa uma ação e retorna se foi executado com sucesso ou não. POST /remoteAction
void postRemoteActions() {
  String postBody = httpRestServer.arg("plain");
  Serial.println("Recebido ==> \n" + postBody + "\n");

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postBody);
  //Verificação da informação e tratamento caso haja erro na requisição
  if (error) {
    Serial.print(F("Error parsing JSON "));
    Serial.println(error.c_str());

    String msg = error.c_str();

    httpRestServer.send(400, F("text/html"), "Error ao dar parse no json recebido! <br>" + msg);

  } else {
    //Colocando as informações recebidas do servidor em formato JSON e identificando qual subrotina foi escolhida pelo usuário
    JsonObject postObj = doc.as<JsonObject>();

    if (httpRestServer.method() == HTTP_POST) {
      if (postObj.containsKey("subFunction")) {

        String subRotina = postObj["subFunction"];

        if (subRotina == "ligarMotor") {
          ligarMotor();
        } else if (subRotina == "desligarMotor") {
          desligarMotor();
        } else if (subRotina == "desligarTorneira") {
          desligarTorneira();
        } else if (subRotina == "ligarTorneira") {
          ligarTorneira();
        } else if (subRotina == "iniciarProcessamento") {
          iniciarProcessamento();
        } else if (subRotina == "desligarProcessamento") {
          desligarProcessamento();
        } else {
          DynamicJsonDocument doc(512);
          doc["isSucess"] = false;
          doc["message"] = "Subrotina inválida!";

          String buf;
          serializeJson(doc, buf);

          httpRestServer.send(422, F("application/json"), buf);
        }

        DynamicJsonDocument doc(512);
        doc["isSucess"] = true;
        doc["message"] = "Subrotina acionada com sucesso!";

        String buf;
        serializeJson(doc, buf);

        httpRestServer.send(201, F("application/json"), buf);
      } else {
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
//Basicamente definindo oq fazer dependendo do endereço que foi acionado
void restServerRouting() {
  //Se alguem chamar /sensorData e a ação HTTP for um GET, usar o controller getSensorData definido anteriormente
  httpRestServer.on(F("/sensorData"), HTTP_GET, getSensorData);

  //Se alguem chamar /systemStatus e a ação HTTP for um GET, usar o controller getSystemStatus definido anteriormente
  httpRestServer.on(F("/systemStatus"), HTTP_GET, getSystemStatus);

   //Se alguem chamar /remoteActions e a ação HTTP for um POST, usar o controller postRemoteActions definido anteriormente
  httpRestServer.on(F("/remoteActions"), HTTP_POST, postRemoteActions);
}

//Programa em loop
void loop()
{
  delay(1000);
  Blynk.run();
  timer.run();
  serverUpdateTimer.run();
  httpRestServer.handleClient();


  // Variavel que seta se a composteira esta ligada ou nao. Acionada via Blynk ou ação remota
  if (isProcessing)
  {
    //controlando o tempo de 72h
    time_t now = DateTime.now();
    int elapsed = now - processingStartTime;
    //Chamando a funcao que vai definir em qual estado estamos, a umidade está ok? a temperatura está ok?
    currentState = getCurrentState(currentState);

    //se tomamos uma ação e ainda continuamos no mesmo lugar (estado), não precisamos tomar a mesma ação novamente
    if (currentState != prevState) {
      Serial.println((String)"Dados atuais:\n Umidade: " + humidity + "\n Temperatura: " + temperature);
      Serial.println();
      Serial.println((String) "Dados de controle atuais:\n Umidade - Máx = " + maxUmid + " Min = " + minUmid);
      Serial.println((String) " Temperatura - Máx = " + maxTemp + " Min = " + minTemp);
      Serial.println();
      
      //Definindo uma estratégia de programação pelo método de estados, onde cada estado representa uma ação requerida do servidor e dos atuadores, como torneira e aquecedores.
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
          // Teoricamente ja esta desligado, forçando o desligamento para prevenir.
          desligarAquecedor();
          ligarTorneira();
          break;

        case 5:
          //"HIGH-TEMP HIGH-HUM"
          Serial.println("Umidade e temperatura acima do máximo.");
          // Teoricamente ja estao desligados, forçando o desligamento para prevenir.
          desligarAquecedor();
          desligarTorneira();
          break;

        case 6:
          Serial.println("Temperatura abaixo do mínimo.");
          // Teoricamente ja estao desligados, forçando o desligamento para prevenir.
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
      //Transformando o estado atual no estado anterior. Assim, é possível comparar os dois estados e identificar caso haja alguma diferença entre eles
      prevState = currentState;
    }

//Quando passadas as 72 horas do processamento
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

//Decisões de seleção de estado
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

//Rotinas de leitura dos sensores de umidade, temperatura e pH
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
{
  for (int i = 0; i < 10; i++) //Pegando uma média de 10 medições
  {
    buf[i] = analogRead(SensorPin);
    delay(10);
  }
  for (int i = 0; i < 9; i++) //Classificando do menor para o maior valor encontrado
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
  for (int i = 2; i < 8; i++) //Excluindo os dois menores e maiores valores
    phMedio += buf[i];
  float phValor = (float)phMedio * 5.0 / 1024 / 6; //Conversão da leitura do sensor em mV
  phValor = 3.5 * phValor; Conversão de mV para pH.

//Contorno do erro na medição de pH
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
void ligarAquecedor()                                                     
{
  digitalWrite(D2, HIGH);                                                 
  delay(10);
  Serial.println((String) "Ligando o aquecedor");
}
void desligarAquecedor()                                                  
{
  digitalWrite(D2, LOW);                                                  
  delay(10);
  Serial.println((String) "desligando o aquecedor");
}

//Identificando ação remota e iniciando o processamento
void iniciarProcessamento() {
  isProcessing = 1;
  Serial.println((String) "Iniciando processamento por ação remota");
}

//Identificando ação remota e parando o processamento
void desligarProcessamento() {
  isProcessing = 0;
  Serial.println((String) "Finalizando processamento por ação remota");
}

//CALLBACKS PINOS VIRTUAIS
BLYNK_WRITE(V3)
{
  minUmid = param.asFloat(); // Get value as integer
  Serial.println((String) "Nova umidade mínima definida " + minUmid + "%");
}

BLYNK_WRITE(V4)
{
  maxUmid = param.asFloat(); // Get value as integer
  Serial.println((String) "Nova umidade máxima definida " + maxUmid + "%");        
}

BLYNK_WRITE(V8)
{
  isProcessing = param.asInt(); // Get value as integer
  processingStartTime = DateTime.now();
  Serial.println((String) "Iniciando processamento...");               
}
