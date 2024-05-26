#include <PubSubClient.h>
#include <WiFi.h>
#include <DHT.h>

#define ON 1
#define OFF 0
#define DHTTYPE DHT22

const char* ssid = "Wokwi-GUEST";
const char* password = "";
WiFiClient espClient;

const char* mqttServer = "mqtt.netpie.io";
const int mqttPort = 1883;
const char* clientID = "93c35b34-a81f-460a-9f84-012b4d267221";
const char* mqttUser = "99wVJvnLUaYN6RD8goHSwrZnCY3okhsF";
const char* mqttPassword = "3bjr8T86aZSfCifqrnu2uP8EMbSF3k8Y";
const char* topic_pub = "@shadow/data/update";
const char* topic_sub = "@msg/lab_ict_kps/command";

PubSubClient client(espClient);

const int RED_LED_PIN = 32;
const int GREEN_LED_PIN = 33;
const int BUZZER_PIN = 25;
const int YELLOW_LED_ALERT = 27;
const int ORANGE_LED_ALERT = 14;
const int DHT_PIN = 26;
DHT dht(DHT_PIN, DHTTYPE);
const int LDR_SENSOR = 13;
const int TEMP_SENSOR = 12;

int luxThreshold = 100;
int redLED_status = OFF, greenLED_status = OFF, buzzer_status = OFF;

const int trigPin = 2;
const int echoPin = 4;
const int ledPin = 13;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  char mqttinfo[80];
  snprintf(mqttinfo, 75, "Attempting MQTT connection at %s:%d (%s/%s)...", mqttServer, mqttPort, mqttUser, mqttPassword);
  while (!client.connected()) {
    Serial.println(mqttinfo);
    String clientId = clientID;
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("...Connected");
      client.subscribe(topic_sub);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void messageReceivedCallback(char* topic, byte* payload, unsigned int length) {
  char payloadMsg[80];

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    payloadMsg[i] = (char)payload[i];
  }
  payloadMsg[length] = '\0';
  Serial.println();
  Serial.println("-----------------------");
  processMessage(payloadMsg);
}

void processMessage(String recvCommand) {
  if (recvCommand == "RED_ON") {
    digitalWrite(RED_LED_PIN, HIGH);
    redLED_status = ON;
  } else if (recvCommand == "RED_OFF") {
    digitalWrite(RED_LED_PIN, LOW);
    redLED_status = OFF;
  } else if (recvCommand == "GREEN_ON") {
    digitalWrite(GREEN_LED_PIN, HIGH);
    greenLED_status = ON;
  } else if (recvCommand == "GREEN_OFF") {
    digitalWrite(GREEN_LED_PIN, LOW);
    greenLED_status = OFF;
  } else if (recvCommand == "BUZZER_ON") {
    buzzer_status = ON;
  } else if (recvCommand == "BUZZER_OFF") {
    buzzer_status = OFF;
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(messageReceivedCallback);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  redLED_status = OFF;
  greenLED_status = OFF;
  buzzer_status = OFF;
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  tone(BUZZER_PIN, 100, 1000);

  pinMode(YELLOW_LED_ALERT, OUTPUT);
  digitalWrite(YELLOW_LED_ALERT, LOW);
  pinMode(ORANGE_LED_ALERT, OUTPUT);
  digitalWrite(YELLOW_LED_ALERT, LOW);

  pinMode(LDR_SENSOR, INPUT);
  pinMode(TEMP_SENSOR, OUTPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  dht.read();
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance_cm = duration * 0.034 / 2;

  String distanceStatus;

  if (distance_cm < 50) {
    distanceStatus = "Too Close";
    digitalWrite(ORANGE_LED_ALERT, LOW);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_ALERT, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
  } else if (distance_cm < 100) {
    distanceStatus = "Near";
    digitalWrite(ORANGE_LED_ALERT, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_ALERT, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
  } else if (distance_cm < 200) {
    distanceStatus = "Safe";
    digitalWrite(ORANGE_LED_ALERT, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_ALERT, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
  } else if (distance_cm < 300) {
    distanceStatus = "Far";
    digitalWrite(ORANGE_LED_ALERT, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_ALERT, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
  } else if (distance_cm < 400) {
    distanceStatus = "Very Far";
    digitalWrite(ORANGE_LED_ALERT, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_ALERT, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
  }

  String publishMessage = "{\"data\": {\"buzzer\": " + String(buzzer_status) +
                          ",\"temperature\": " + String(t) +
                          ", \"humidity\": " + String(h) +
                          ", \"ultrasonic_distance\": " + String(distance_cm) +
                          ", \"distance_status\": \"" + distanceStatus + "\"}}";

  Serial.println(publishMessage);
  client.publish(topic_pub, publishMessage.c_str());

  if (buzzer_status == ON) {
    tone(BUZZER_PIN, 100, 1000);
  } else if (buzzer_status == OFF) {
    noTone(BUZZER_PIN);
  }

  delay(2000);
}

