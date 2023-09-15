#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <LittleFS.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <PubSubClient.h>
#include <Wire.h>

AsyncWebServer server(80);

const char* ssid = "ESP32-Access-Point";
const char* password = "123456789";

const char* ssidhs = "ESP32-Access-Point";
const char* passwordhs = "123456789";

const int Entrada_configuracion = 13;
const int Entrada_WD = 4;

unsigned long previousMillis = 0;

// Salidas Digitales

const int Salida_1 = 14;
const int Salida_2 = 12;
const int Salida_WD = 5;



// Inicialización Señales Digitales Entrada
int Valor_entrada_configuracion = 0;
int Valor_Entrada_WD = 0;

int cambio_eWD = 1;

// Nombres equipos y señales
int numero_puerto_MQTT = 1883;
const char* nmqtt = "ptg43.mooo.com";

const char* nombre_dispositivo = "wemo_1";
const char* nombre_salida_1 = "Salida_1";
const char* nombre_salida_2 = "Salida_2";
const char* nombre_salida_WD = "Salida_WD";
const char* nombre_entrada_WD = "Entrada_WD";


String nombre_completo_salida_1 = "Salida_1";
String nombre_completo_salida_2 = "Salida_2";
String nombre_completo_salida_WD = "Salida_WD";
String nombre_completo_entrada_WD = "Entrada_WD";

boolean conf = false;

// Valores funcionamiento normal wifi y mqtt
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

// HTML web page to handle 3 input fields (inputString, inputInt, inputFloat)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <form action="/get" target="hidden-form">
    SSID WIFI (Valor actual: %inputssid%): <input type="text" name="inputssid">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Contrasena WIFI (Valor actual: %inputpassword%): <input type="text " name="inputpassword">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>

  <form action="/get" target="hidden-form">
    Servidor MQTT (Valor actual: %servidor_MQTT%): <input type="text " name="servidor_MQTT">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Puerto MQTT (Valor actual: %puerto_MQTT%): <input type="text " name="puerto_MQTT">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Nombre Dispositivo (Valor actual: %dispositivo%): <input type="text " name="dispositivo">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Nombre Salida 1 (Valor actual: %salida_1%): <input type="text " name="salida_1">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
    <form action="/get" target="hidden-form">
    Nombre Salida 2 (Valor actual: %salida_2%): <input type="text " name="salida_2">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
    <form action="/get" target="hidden-form">
    Nombre Salida 3 (Valor actual: %salida_WD%): <input type="text " name="salida_WD">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void IRAM_ATTR Entrada_WD_interrupt() {
  cambio_eWD = 1;
}

void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS& fs, const char* path) {
  Serial.printf("Reading file: %s\r\n", path);
  File file = LittleFS.open(path, "r");
  if (!file || file.isDirectory()) {
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while (file.available()) {
    fileContent += String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS& fs, const char* path, const char* message) {
  Serial.printf("Writing file: %s\r\n", path);
  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

String processor(const String& var) {
  Serial.println(var);
  if (var == "inputssid") {
    return readFile(SPIFFS, "/inputssid.txt");
  } else if (var == "inputpassword") {
    return readFile(SPIFFS, "/inputpassword.txt");
  } else if (var == "servidor_MQTT") {
    return readFile(SPIFFS, "/servidor_MQTT.txt");
  } else if (var == "puerto_MQTT") {
    return readFile(SPIFFS, "/puerto_MQTT.txt");
  } else if (var == "dispositivo") {
    return readFile(SPIFFS, "/dispositivo.txt");
  } else if (var == "salida_1") {
    return readFile(SPIFFS, "/salida_1.txt");
  } else if (var == "salida_2") {
    return readFile(SPIFFS, "/salida_2.txt");
  } else if (var == "salida_WD") {
    return readFile(SPIFFS, "/salida_WD.txt");
  }
  return String();
}

void setup_wifi() {
  delay(10);

  Serial.println();
  Serial.print("Conectando a red: ");

  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
  }
  Serial.println("");
  Serial.println("WiFi Conectada");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
}


void s1_off() {
  String s1c = nombre_completo_salida_1 + "c";
  Serial.println("Cambiando " + nombre_completo_salida_1 + " a: off");
  digitalWrite(Salida_1, LOW);
  //client.publish(nombre_completo_salida_1.c_str(), "off");
  client.publish("Puerta/Estado", "Apertura Apagada");
  client.publish(s1c.c_str(), "off");
}

void s1_on() {
  String s1c = nombre_completo_salida_1 + "c";
  Serial.println("Cambiando " + nombre_completo_salida_1 + " a: on");
  digitalWrite(Salida_1, HIGH);
  //client.publish(nombre_completo_salida_1.c_str(), "on");
  client.publish("Puerta/Estado", "Apertura Activa");
  client.publish(s1c.c_str(), "on");
  delay(1000);
  s1_off();
}

void s2_off() {
  String s1c = nombre_completo_salida_2 + "c";
  Serial.println("Cambiando " + nombre_completo_salida_2 + " a: off");
  digitalWrite(Salida_2, LOW);
  //client.publish(nombre_completo_salida_2.c_str(), "off");
  client.publish("Puerta/Estado", "Cierre Apagado");
  client.publish(s1c.c_str(), "off");
}

void s2_on() {
  String s1c = nombre_completo_salida_2 + "c";
  Serial.println("Cambiando " + nombre_completo_salida_2 + " a: on");
  digitalWrite(Salida_2, HIGH);
  //client.publish(nombre_completo_salida_2.c_str(), "on");
  client.publish("Puerta/Estado", "Cierre Activo");
  client.publish(s1c.c_str(), "on");
  delay(1000);
  s2_off();
}

void s3_on() {
  String s1c = nombre_completo_salida_WD + "c";
  Serial.println("Cambiando " + nombre_completo_salida_WD + " a: on");
  digitalWrite(Salida_WD, HIGH);
  //client.publish(nombre_completo_salida_WD.c_str(), "on");
  client.publish("Puerta/Estado", "WD Activo");
  client.publish(s1c.c_str(), "on");
}

void s3_off() {
  String s1c = nombre_completo_salida_WD + "c";
  Serial.println("Cambiando " + nombre_completo_salida_WD + " a: off");
  digitalWrite(Salida_WD, LOW);
  //client.publish(nombre_completo_salida_WD.c_str(), "off");
  client.publish("Puerta/Estado", "WD Apagado");
  client.publish(s1c.c_str(), "off");
}


void callback(char* topic, byte* message, unsigned int length) {
  Serial.println("");
  Serial.print("Mensaje recivido en topic: ");
  Serial.println(topic);
  Serial.println("Mensaje: ");

  String mensaje;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    mensaje += (char)message[i];
  }
  Serial.println();

  if (String(topic) == nombre_completo_salida_1) {
    if (mensaje == "on") {
      s1_on();
    } else if (mensaje == "off") {
      s1_off();
    }
  } else if (String(topic) == nombre_completo_salida_2) {
    if (mensaje == "on") {
      s2_on();
    } else if (mensaje == "off") {
      s2_off();
    }
  } else if (String(topic) == nombre_completo_salida_WD) {
    if (mensaje == "on") {
      s3_on();
    } else if (mensaje == "off") {
      s3_off();
    }
  } else {
    client.publish("Puerta/Estado", "mensaje no atendido");
  }
}

void reconnect() {
  // Loop until we're reconnected

  while (!client.connected()) {
    Serial.print("Conectando a servidor MQTT...");
    // Attempt to connect
    String clientId = "WemoClient-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado");
      client.publish("Puerta/Estado", "MQTT conectado");

      Serial.println("");
      Serial.println("Salidas");
      Serial.println(nombre_completo_salida_1);
      Serial.println(nombre_completo_salida_2);
      Serial.println(nombre_completo_salida_WD);
      Serial.println(nombre_completo_entrada_WD);

      Serial.println("");

      client.subscribe(nombre_completo_salida_1.c_str(), 1);
      client.subscribe(nombre_completo_salida_2.c_str(), 1);
      client.subscribe(nombre_completo_salida_WD.c_str(), 1);

    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(" reintantando en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(Entrada_configuracion, INPUT);
  pinMode(Salida_1, OUTPUT);
  pinMode(Salida_2, OUTPUT);
  pinMode(Salida_WD, OUTPUT);
  digitalWrite(Salida_1, LOW);
  digitalWrite(Salida_2, LOW);
  digitalWrite(Salida_WD, LOW);


  Valor_entrada_configuracion = digitalRead(Entrada_configuracion);

  // Initialize SPIFFS
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  if (Valor_entrada_configuracion == HIGH) {
    conf = true;
  } else {
    conf = false;
  }

  if (conf == true) {
    // Inicializa Hotspot
    WiFi.softAP(ssidhs, passwordhs);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Send web page with input fields to client
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send_P(200, "text/html", index_html, processor);
    });

    // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest* request) {
      String inputMessage;
      // GET paramssid value on <ESP_IP>/get?inputssid=<inputMessage>
      if (request->hasParam("inputssid")) {
        inputMessage = request->getParam("inputssid")->value();
        writeFile(SPIFFS, "/inputssid.txt", inputMessage.c_str());
      }
      // GET parampassword value on <ESP_IP>/get?inputpassword=<inputMessage>
      else if (request->hasParam("inputpassword")) {
        inputMessage = request->getParam("inputpassword")->value();
        writeFile(SPIFFS, "/inputpassword.txt", inputMessage.c_str());
      } else if (request->hasParam("servidor_MQTT")) {
        inputMessage = request->getParam("servidor_MQTT")->value();
        writeFile(SPIFFS, "/servidor_MQTT.txt", inputMessage.c_str());
      } else if (request->hasParam("puerto_MQTT")) {
        inputMessage = request->getParam("puerto_MQTT")->value();
        writeFile(SPIFFS, "/puerto_MQTT.txt", inputMessage.c_str());
      } else if (request->hasParam("dispositivo")) {
        inputMessage = request->getParam("dispositivo")->value();
        writeFile(SPIFFS, "/dispositivo.txt", inputMessage.c_str());
      } else if (request->hasParam("salida_1")) {
        inputMessage = request->getParam("salida_1")->value();
        writeFile(SPIFFS, "/salida_1.txt", inputMessage.c_str());
      } else if (request->hasParam("salida_2")) {
        inputMessage = request->getParam("salida_2")->value();
        writeFile(SPIFFS, "/salida_2.txt", inputMessage.c_str());
      } else if (request->hasParam("salida_WD")) {
        inputMessage = request->getParam("salida_WD")->value();
        writeFile(SPIFFS, "/salida_WD.txt", inputMessage.c_str());
      } else {
        inputMessage = "No message sent";
      }
      Serial.println(inputMessage);
      request->send(200, "text/text", inputMessage);
    });
    server.onNotFound(notFound);
    server.begin();
  }

  else {

    String ssida = readFile(SPIFFS, "/inputssid.txt");
    String passworda = readFile(SPIFFS, "/inputpassword.txt");
    String servidor_MQTTa = readFile(SPIFFS, "/servidor_MQTT.txt");
    String puerto_MQTTa = readFile(SPIFFS, "/puerto_MQTT.txt");
    String dispositivoa = readFile(SPIFFS, "/dispositivo.txt");
    String salida_1a = readFile(SPIFFS, "/salida_1.txt");
    String salida_2a = readFile(SPIFFS, "/salida_2.txt");
    String salida_WDa = readFile(SPIFFS, "/salida_WD.txt");

    ssid = ssida.c_str();
    password = passworda.c_str();
    int str_len = servidor_MQTTa.length() + 1;
    char n1[str_len];
    servidor_MQTTa.toCharArray(n1, str_len);

    Serial.print("n1: ");
    Serial.println(n1);

    numero_puerto_MQTT = puerto_MQTTa.toInt();
    nombre_dispositivo = dispositivoa.c_str();
    nombre_salida_1 = salida_1a.c_str();
    nombre_salida_2 = salida_2a.c_str();
    nombre_salida_WD = salida_WDa.c_str();

    setup_wifi();

    nombre_completo_salida_1 = String(nombre_dispositivo) + "/" + String(nombre_salida_1);
    nombre_completo_salida_2 = String(nombre_dispositivo) + "/" + String(nombre_salida_2);
    nombre_completo_salida_WD = String(nombre_dispositivo) + "/" + String(nombre_salida_WD);
    nombre_completo_entrada_WD = String(nombre_dispositivo) + "/" + String(nombre_entrada_WD);

    client.setServer(nmqtt, numero_puerto_MQTT);
    client.setCallback(callback);

    // pinMode(Salida_1, OUTPUT);
    // pinMode(Salida_2, OUTPUT);
    // pinMode(Salida_WD, OUTPUT);
    // pinMode(Entrada_configuracion, INPUT);

    attachInterrupt(Entrada_WD, Entrada_WD_interrupt, RISING);

    // digitalWrite(Salida_1, LOW);
    // digitalWrite(Salida_2, HIGH);
    // digitalWrite(Salida_WD, LOW);

    // delay(2000);
    // digitalWrite(Salida_2, LOW);
  }
}

void loop() {

  Valor_entrada_configuracion = digitalRead(Entrada_configuracion);

  if (Valor_entrada_configuracion == true) {

    Serial.println("En Configuración");
    delay(1000);
  } else {

    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    long now = millis();
    if (now - lastMsg > 1000) {
      lastMsg = now;
      if (cambio_eWD == 1) {
        Serial.println("EWD actualizada");
        int val = 0;
        char cadena[1];
        val = digitalRead(Entrada_WD);
        dtostrf(val, 1, 0, cadena);
        client.publish(nombre_completo_entrada_WD.c_str(), cadena);
        cambio_eWD = 0;
        delay(5000);
        s3_off();
        val = digitalRead(Entrada_WD);
        dtostrf(val, 1, 0, cadena);
        client.publish(nombre_completo_entrada_WD.c_str(), cadena);
      }
    }
  }
}