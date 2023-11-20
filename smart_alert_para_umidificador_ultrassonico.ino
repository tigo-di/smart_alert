//Setup for MQTT and WiFi============================
#include <ESP8266WiFi.h>
//Library for MQTT:
#include <PubSubClient.h>
//Library for Json format using version 5:
#include <ArduinoJson.h>

#define sensorLiquid D2
#define buzzer D6

// declare topic for publish message
const char* topic_pub = "ESP_Pub";

// declare topic for subscribe message
const char* topic_sub = "ESP_Sub";

const char* currentDevice = "nodemcu-esp-12e";

// Update these with values suitable for your network.
const char* ssid = "nome-da-rede-wireless";
const char* password = "password";
const char* mqtt_server = "ip";

int hasLiquid;
int playBuzzer;
int playBuzzerTimes = 0;
bool sendAlertToBroken = true;

int timesPlayBuzzer = 3;
int intervalAfterPlayBuzzer = 1000;//5000;

WiFiClient espClient;
PubSubClient client(espClient);


void setup_wifi() {
  delay(100);
  // We start by connecting to a WiFi network
  Serial.print("Conectando na rede Wireless ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("Conectado na rede Wireless!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}



void callback(char* topic, byte* payload, unsigned int length)
{
  //Receiving message as subscriber
  Serial.print("Nova mensagem no tópico: ");
  Serial.println(topic);
  String json_received;
  Serial.print("JSON Recebido:");

  for (int i = 0; i < length; i++) {
    json_received += ((char)payload[i]);
    //Serial.print((char)payload[i]);
  }
  Serial.println(json_received);
  //if receive ask status from node-red, send current status
  if (json_received == "status")
  {
    check_stat();
  }
  else
  {
    //Parse json
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(json_received);

    //get json parsed value
    //sample of json: {"device":"Lamp1","trigger":"on"}
    Serial.print("Command:");
    String device = root["device"];
    String trigger = root["trigger"];
    Serial.println("Turn " + trigger + " " + device);
    Serial.println("-----------------------");
    //Trigger device
    //Buzzer***************************
    if (device == "buzzer")
    {
      if (trigger == "on")
      {
        tone(buzzer, 4200, 250);
      }
    }
    // check_stat();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Tentando reconectar ao Broken...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("conectado");
      //once connected to MQTT broker, subscribe command if any
      client.subscribe(topic_sub);
      check_stat();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" tentando reconectar em 5 segundos");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}




void setup()
{
  delay(1000); // Evitando variações do boot do NodeMCU
  Serial.begin(9600);
  
  pinMode(D2, INPUT_PULLUP);
  pinMode(D6, OUTPUT);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //subscribe topic
  client.subscribe(topic_sub);
}

void check_stat()
{
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  bool hasLiquid = digitalRead(sensorLiquid) <= 0;

  if (hasLiquid)
  {
    JSONencoder["message"] = "Não é preciso repor água nesse momento";
  }
  else
  {
    JSONencoder["message"] = "Repor água do reservatório do umidificador";
  }


  JSONencoder["device"] = currentDevice;
  
  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println("Enviando mensagem via MQTT para tópico no Broken..");
  Serial.println(JSONmessageBuffer);
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  if (client.publish(topic_pub, JSONmessageBuffer) == true) {
    Serial.println("Mensagem enviada para o tópico com sucesso");
  } else {
    Serial.println("Erro no envio de mensagem para o tópico");
  }

}



void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  //  Faz a leitura do sensor de liquido
  hasLiquid = digitalRead(sensorLiquid) <= 0;
  
  //  Mostra no Serial Monitor se há líquido ou não detectado no nível do sensor
  Serial.print("Repor água?: ");
  Serial.println(hasLiquid ? "não":"SIM");

  if (hasLiquid) {
    playBuzzerTimes = 0;
    sendAlertToBroken = true;
  }
  else
  {

    if(sendAlertToBroken) {
      sendAlertToBroken = false;
      Serial.println("Enviando mensagem");
      alertToBroken();
    }



    if (playBuzzerTimes < timesPlayBuzzer) {
            
      playBuzzerTimes = playBuzzerTimes + 1;
      
      Serial.print("Buzzer acionado: ");
      Serial.print(playBuzzerTimes);
      Serial.println("x");
            
      tone(buzzer, 4200, 250);
      delay(intervalAfterPlayBuzzer);

    }

  }

  
  delay(400);
}


void alertToBroken()
{
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["device"] = currentDevice;
  JSONencoder["message"] = "Repor água do reservatório do umidificador";
  
  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println("Enviando mensagem via MQTT para tópico no Broken..");
  Serial.println(JSONmessageBuffer);
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  if (client.publish(topic_pub, JSONmessageBuffer) == true) {
    Serial.println("Mensagem enviada para o tópico com sucesso");
  } else {
    Serial.println("Erro no envio de mensagem para o tópico");
  }


  Serial.println("-------------");
}



