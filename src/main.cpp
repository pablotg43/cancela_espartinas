#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <LittleFS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <Wire.h>

AsyncWebServer server(80);

const char *ssid = "ESP32-Access-Point";
const char *password = "123456789";

const char *ssidhs = "ESP32-Access-Point";
const char *passwordhs = "123456789";

// Entradas Digitales
const int Entrada_configuracion = 14;
int Valor_entrada_configuracion = 0;
const int Entradas[2] = {4,5};

// Salidas Digitales
const int Salida[2] = {13,12};

enum Estados {
  Abriendo_señal_activa,
  Cerrando_señal_activa,
  Abierta,
  Cerrada,
  Abierta_con_deteccion,
  Abriendo_con_deteccion,
  Configuracion,
  Abriendo,
  Cerrando,
  Abriendo_con_deteccion_y_orden
};

Estados estado = Estados::Cerrada;

// Inicialización Variables
unsigned long horas = 3600000;
unsigned long minutos = 60000;
unsigned long segundos = 1000;

//Temporizadores
unsigned long now = 0;
unsigned long temp_Tcomms = 0;
unsigned long Temp_T1 = 0;
unsigned long Temp_T2 = 0;
unsigned long Temp_T3 = 0;
unsigned long Temp_Tapertura = 0;
unsigned long Temp_Tcierre = 0;
unsigned long Temp_T4 = 0;

//Tiempos
unsigned long Tapertura = 10000;
unsigned long Tcierre = 10000;
unsigned long T1 = 10000; // Cierre después de detección
unsigned long T2 = 10000; // Cierre sin detección 
unsigned long T3 = 10000; // Autocierre
unsigned long T4 = 3000;  // Tiempo Activación Señales
unsigned long Tcomms = 10000;  // Tiempo Comunicaciones

//Ordenes
boolean ordenes[2] = {false,false};
boolean entradas[2] = {false,false};

//Varios
boolean cambio_estado=false;
String s1c[2] = {"s1", "s2"};
String s1ctext[2] = {"off", "off"};
String sa="";
String sb[5]={"","","","",""};
String sc[2]={"",""};
String sd[2]={"",""};

// Nombres equipos y señales
int numero_puerto_MQTT = 1883;
const char *nmqtt = "ptg43.mooo.com";
const char *nombre_dispositivo = "wemo_cancela_1";
String nombre_estado = "Salida_1";
String nombre_completo_ordenes[2] = {"Movimiento", "Cierre"};
String nombre_completo_entrada_configuracion = "Entrada_Configuracion";
String nombre_completo_temporizadores[5] = {"Tapertura", "Tcierre", "T1", "T2", "T3"};
String nombre_completo_entradas[2] = {"Celula", "Reserva"};
boolean conf = false;


// Valores funcionamiento normal wifi y mqtt
WiFiClient espClient;
PubSubClient client(espClient);
char msg[50];
int value = 0;

#include <servidor.txt>

void proximoEstado() 
{
    switch (estado) {
        case Estados::Cerrando_señal_activa:
            // Serial.println("Estado Cerrando_señal_activa");
            if (now - Temp_T4 > T4) { estado=Estados::Cerrando; cambio_estado=true; Temp_Tcierre=now;  }
            else if (ordenes[0]) {estado=Estados::Abriendo_señal_activa; cambio_estado=true; Temp_T4=now;}
            else if (entradas[0]) {estado=Estados::Abriendo_con_deteccion_y_orden; cambio_estado=true; Temp_T4=now;}
        break;
        case Estados::Cerrada:
            // Serial.println("Estado Cerrada");
            if (now - Temp_T3 > T3) { estado=Estados::Cerrada; cambio_estado=true; Temp_T3=now;  }
            else if (ordenes[0]) {estado=Estados::Abriendo_señal_activa; cambio_estado=true; Temp_T4=now;}
        break;
        case Estados::Cerrando:
            // Serial.println("Estado Cerrando");
            if (now - Temp_Tcierre > Tcierre) { estado=Estados::Cerrada; cambio_estado=true; Temp_T3=now;  }
            else if (ordenes[0]) {estado=Estados::Abriendo_señal_activa; cambio_estado=true; Temp_T4=now;}
            else if (entradas[0]) {estado=Estados::Abriendo_con_deteccion_y_orden; cambio_estado=true; Temp_T4=now;}
        break;        
        case Estados::Abriendo_señal_activa:
            // Serial.println("Estado Abriendo_señal_activa");
            if (now - Temp_T4 > T4) { estado=Estados::Abriendo; cambio_estado=true; Temp_Tapertura=now;  }
            else if (ordenes[1]) {estado=Estados::Cerrando_señal_activa; cambio_estado=true; Temp_T4=now;}
            else if (entradas[0]) {estado=Estados::Abriendo_con_deteccion_y_orden; cambio_estado=true; Temp_T4=now;}
        break;   
        case Estados::Abierta:
            // Serial.println("Estado Abierta");
            if (now - Temp_T2 > T2) { estado=Estados::Cerrando_señal_activa; cambio_estado=true; Temp_T4=now;  }
            else if (entradas[0]) {estado=Estados::Abierta_con_deteccion; cambio_estado=true; Temp_T1=now;}
            else if (ordenes[1]) {estado=Estados::Cerrando_señal_activa; cambio_estado=true; Temp_T4=now;}
        break; 
        case Estados::Abriendo:
            // Serial.println("Estado Abriendo");
            if (now - Temp_Tapertura > Tapertura) { estado=Estados::Abierta; cambio_estado=true; Temp_T2=now;  }
            else if (ordenes[1]) {estado=Estados::Cerrando_señal_activa; cambio_estado=true; Temp_T4=now;}
            else if (entradas[0]) {estado=Estados::Abriendo_con_deteccion; cambio_estado=true; Temp_Tapertura=now;}
        break;          
        case Estados::Abierta_con_deteccion:
            // Serial.println("Estado Abierta_con_deteccion");
            if (entradas[0]) {estado=Estados::Abierta_con_deteccion; cambio_estado=false; Temp_T1=now;}
            else if (now - Temp_T1 > T1) { estado=Estados::Cerrando_señal_activa; cambio_estado=true; Temp_T4=now;  }
        break;   
        case Estados::Abriendo_con_deteccion:
            // Serial.println("Estado Abriendo_con_deteccion");
            if (now - Temp_Tapertura > Tapertura) { estado=Estados::Abierta_con_deteccion; cambio_estado=true; Temp_T1=now;  }
        break;       
        case Estados::Abriendo_con_deteccion_y_orden:
            // Serial.println("Estado Abriendo_con_deteccion_y_orden");
            if (now - Temp_T4 > T4) { estado=Estados::Abriendo_con_deteccion; cambio_estado=true; Temp_Tapertura=now;  }
        break;   
        case Estados::Configuracion:
        break;
    }
}

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);
    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory())
    {
        Serial.println("- empty file or failed to open file");
        return String();
    }
    Serial.println("- read from file:");
    String fileContent;
    while (file.available())
    {
        fileContent += String((char)file.read());
    }
    file.close();
    Serial.println(fileContent);
    return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);
    File file = LittleFS.open(path, "w");
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
    }
    file.close();
}

String processor(const String &var)
{
    Serial.print("var: ");
    Serial.println(var);

    if (var == "inputssid")
    {
        return readFile(LittleFS, "/inputssid.txt");
    }
    else if (var == "inputpassword")
    {
        return readFile(LittleFS, "/inputpassword.txt");
    }
    else if (var == "servidor_MQTT")
    {
        return readFile(LittleFS, "/servidor_MQTT.txt");
    }
    else if (var == "puerto_MQTT")
    {
        return readFile(LittleFS, "/puerto_MQTT.txt");
    }
    else if (var == "dispositivo")
    {
        return readFile(LittleFS, "/dispositivo.txt");
    }
    else if (var == "T1")
    {
        return readFile(LittleFS, "/T1.txt");
    }
    else if (var == "T2")
    {
        return readFile(LittleFS, "/T2.txt");
    }
    else if (var == "T3")
    {
        return readFile(LittleFS, "/T3.txt");
    }
    else if (var == "Tapertura")
    {
        return readFile(LittleFS, "/Tapertura.txt");
    }
    else if (var == "Tcierre")
    {
        return readFile(LittleFS, "/Tcierre.txt");
    }

    else if (var == "estado_wifi")
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            return "Conectada";
        }
        else
        {
            return "Desconectada";
        }
    }
    else if (var == "estado_MQTT")
    {
        if (client.connected())
        {
            return "Conectado";
        }
        else
        {
            return "Desconectado";
        }
    }
    return String();
}

void setup_wifi()
{
    delay(10);

    Serial.println();
    Serial.print("Conectando a red: ");

    Serial.println(ssid);
    WiFi.hostname(nombre_dispositivo);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(250);
    }
    Serial.println("");
    Serial.println("WiFi Conectada");
    Serial.println("Direccion IP: ");
    Serial.println(WiFi.localIP());
}

void s1(int i, boolean estado)
{
    ordenes[i]=estado;
}

void connect(const String& host, int port) 
{ 
    static char pHost[64] = {0}; 
    strcpy(pHost, host.c_str()); 
    client.setServer(pHost, port); 
}

void callback(char *topic, byte *message, unsigned int length)
{
    Serial.println("");
    Serial.print("Mensaje recibido en topic: ");
    Serial.println(topic);
    Serial.println("Mensaje: ");

    String mensaje;

    for (unsigned int i = 0; i < length; i++)
    {
        Serial.print((char)message[i]);
        mensaje += (char)message[i];
    }

    if (String(topic) == nombre_completo_ordenes[0])
    {
        if (mensaje == "on")
        {
            s1(0,true);
        }

        String stopc = nombre_completo_ordenes[0] + "c";
        client.publish(stopc.c_str(), "off");
    }
    
    if (String(topic) == nombre_completo_ordenes[1])
    {
        if (mensaje == "on")
        {
            s1(1,true);
        }
        String stopc = nombre_completo_ordenes[1] + "c";
        client.publish(stopc.c_str(), "off");
    }

    if (String(topic) == nombre_completo_temporizadores[0])
    {
        int duracion = mensaje.toInt();
        String nombre = "Tapertura.txt";
        Tapertura = duracion * segundos;
        Serial.println("Tapertura: " + Tapertura);
        writeFile(LittleFS, nombre.c_str(), mensaje.c_str());
    }

    if (String(topic) == nombre_completo_temporizadores[1])
    {
        int duracion = mensaje.toInt();
        String nombre = "Tcierre.txt";
        Tcierre = duracion * segundos;
        Serial.println("Tcierre: " + Tcierre);
        writeFile(LittleFS, nombre.c_str(), mensaje.c_str());    }

    if (String(topic) == nombre_completo_temporizadores[2])
    {
        int duracion = mensaje.toInt();
        String nombre = "T1.txt";
        T1 = duracion * segundos;
        Serial.println("T1: " + T1);
        writeFile(LittleFS, nombre.c_str(), mensaje.c_str());    }

    if (String(topic) == nombre_completo_temporizadores[3])
    {
        int duracion = mensaje.toInt();
        String nombre = "T2.txt";
        T2 = duracion * segundos;
        Serial.println("T2: " + T2);
        writeFile(LittleFS, nombre.c_str(), mensaje.c_str());    }
    
    if (String(topic) == nombre_completo_temporizadores[4])
    {
        int duracion = mensaje.toInt();
        String nombre = "T3.txt";
        T3 = duracion * minutos;
        Serial.println("T3: " + T3);
        writeFile(LittleFS, nombre.c_str(), mensaje.c_str());    }
}

void reconnect()
{
    // Loop until we're reconnected

    while (!client.connected())
    {
        Serial.print("Conectando a servidor MQTT...");
        // Attempt to connect
        String clientId = "WemoClient-";
        clientId += String(random(0xffff), HEX);

        if (client.connect(clientId.c_str()))
        {
            Serial.println("Conectado");

            Serial.println("");
            Serial.println("Ordenes");
            for (int i = 0; i <= 1; i++)
            {
                Serial.println(nombre_completo_ordenes[i]);
            }
            for (int i = 0; i <= 1; i++)
            {
                Serial.println(nombre_completo_entradas[i]);
            }            
            for (int i = 0; i <= 4; i++)
            {
                Serial.println(nombre_completo_temporizadores[i]);
            }            

            Serial.println(nombre_estado);

            Serial.println("");

            client.subscribe(nombre_completo_ordenes[0].c_str(), 1);
            client.subscribe(nombre_completo_ordenes[1].c_str(), 1);
            client.subscribe(nombre_completo_temporizadores[0].c_str(), 1);
            client.subscribe(nombre_completo_temporizadores[1].c_str(), 1);
            client.subscribe(nombre_completo_temporizadores[2].c_str(), 1);
            client.subscribe(nombre_completo_temporizadores[3].c_str(), 1);
            client.subscribe(nombre_completo_temporizadores[4].c_str(), 1);
        }

        else
        {
            Serial.print("Fallo, rc=");
            Serial.print(client.state());
            Serial.println(" reintantando en 5 segundos");
            delay(5000);
        }
    }
}

void servidorhttp()
{

    // Send web page with input fields to client
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html, processor); });

    // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      String inputMessage;
      // GET paramssid value on <ESP_IP>/get?inputssid=<inputMessage>
      if (request->hasParam("inputssid")) {
        inputMessage = request->getParam("inputssid")->value();
        writeFile(LittleFS, "/inputssid.txt", inputMessage.c_str());
      }
      // GET parampassword value on <ESP_IP>/get?inputpassword=<inputMessage>
      else if (request->hasParam("inputpassword")) {
        inputMessage = request->getParam("inputpassword")->value();
        writeFile(LittleFS, "/inputpassword.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("servidor_MQTT")) {
        inputMessage = request->getParam("servidor_MQTT")->value();
        writeFile(LittleFS, "/servidor_MQTT.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("puerto_MQTT")) {
        inputMessage = request->getParam("puerto_MQTT")->value();
        writeFile(LittleFS, "/puerto_MQTT.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("dispositivo")) {
        inputMessage = request->getParam("dispositivo")->value();
        writeFile(LittleFS, "/dispositivo.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("T1")) {
        inputMessage = request->getParam("T1")->value();
        T1=inputMessage.toInt()*segundos;
        writeFile(LittleFS, "/T1.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("T2")) {
        inputMessage = request->getParam("T2")->value();
        T2=inputMessage.toInt()*segundos;
        writeFile(LittleFS, "/T2.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("T3")) {
        inputMessage = request->getParam("T3")->value();
        T3=inputMessage.toInt()*minutos;
        writeFile(LittleFS, "/T3.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("Tapertura")) {
        inputMessage = request->getParam("Tapertura")->value();
        Tapertura=inputMessage.toInt()*segundos;
        writeFile(LittleFS, "/Tapertura.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("Tcierre")) {
        inputMessage = request->getParam("Tcierre")->value();
        Tcierre=inputMessage.toInt()*segundos;
        writeFile(LittleFS, "/Tcierre.txt", inputMessage.c_str());
      } 
      else if (request->hasParam("reiniciar")) {
        inputMessage = request->getParam("reiniciar")->value();
        if (inputMessage == "1") {
          ESP.restart();
        }
      } 
      else {
        inputMessage = "No message sent";
      }
      Serial.println(inputMessage);
      request->send(200, "text/text", inputMessage); });
    server.onNotFound(notFound);
    server.begin();
}

// void ICACHE_RAM_ATTR  E0_interr() {
//     entradas[0]=true;
// }

// void ICACHE_RAM_ATTR  E1_interr() {
//     entradas[1]=true;
// }

void setup()
{
    Serial.begin(115200);

    pinMode(Entrada_configuracion, INPUT_PULLUP);

    Valor_entrada_configuracion = digitalRead(Entrada_configuracion);
    Serial.print("Estado configuracion: ");
    Serial.print(Valor_entrada_configuracion);

    // Initialize LittleFS
    if (!LittleFS.begin())
    {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    if (Valor_entrada_configuracion == HIGH)
    {
        conf = true;
    }
    else
    {
        conf = false;
    }

    if (conf == true)
    {
        // Inicializa Hotspot
        for (int i = 0; i <= 1; i++)
        {
            pinMode(ordenes[i], OUTPUT);
            digitalWrite(ordenes[i], LOW);
        }

        WiFi.softAP(ssidhs, passwordhs);
        IPAddress IP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(IP);

        String ssida = readFile(LittleFS, "/inputssid.txt");
        String passworda = readFile(LittleFS, "/inputpassword.txt");
        ssid = ssida.c_str();
        password = passworda.c_str();
        estado=Estados::Configuracion;
        servidorhttp();
    }

    else
    {
        String ssida = readFile(LittleFS, "/inputssid.txt");
        String passworda = readFile(LittleFS, "/inputpassword.txt");
        String servidor_MQTTa = readFile(LittleFS, "/servidor_MQTT.txt");
        String puerto_MQTTa = readFile(LittleFS, "/puerto_MQTT.txt");
        String dispositivoa = readFile(LittleFS, "/dispositivo.txt");
        String T1a = readFile(LittleFS, "/T1.txt");
        String T2a = readFile(LittleFS, "/T2.txt");
        String T3a = readFile(LittleFS, "/T3.txt");
        String Taperturaa = readFile(LittleFS, "/Tapertura.txt");
        String Tcierrea = readFile(LittleFS, "/Tcierre.txt");

        T1 = atol(T1a.c_str());
        T2 = atol(T2a.c_str());
        T3 = atol(T3a.c_str());
        Tapertura = atol(Taperturaa.c_str());
        Tcierre = atol(Tcierrea.c_str());
        T1=T1*segundos;
        T2=T2*segundos;
        T3=T3*minutos;
        Tapertura=Tapertura*segundos;
        Tcierre=Tcierre*segundos;

        ssid = ssida.c_str();
        password = passworda.c_str();
        int str_len = servidor_MQTTa.length() + 1;
        char n1[str_len];
        servidor_MQTTa.toCharArray(n1, str_len);

        Serial.print("n1: ");
        Serial.println(n1);
        
        numero_puerto_MQTT = puerto_MQTTa.toInt();
        nombre_dispositivo = dispositivoa.c_str();

        setup_wifi();

        nombre_completo_ordenes[0] = String(nombre_dispositivo) + "/" + "Movimiento";
        nombre_completo_ordenes[1] = String(nombre_dispositivo) + "/" + "Cierre";
        nombre_completo_entradas[0] = String(nombre_dispositivo) + "/" + "Celula";
        nombre_completo_entradas[1] = String(nombre_dispositivo) + "/" + "Reserva";
        nombre_estado = String(nombre_dispositivo) + "/" + "estado";
        nombre_completo_temporizadores[0] = String(nombre_dispositivo) + "/" + "Tapertura";
        nombre_completo_temporizadores[1] = String(nombre_dispositivo) + "/" + "Tcierre";
        nombre_completo_temporizadores[2] = String(nombre_dispositivo) + "/" + "T1";
        nombre_completo_temporizadores[3] = String(nombre_dispositivo) + "/" + "T2";
        nombre_completo_temporizadores[4] = String(nombre_dispositivo) + "/" + "T3";

        connect(servidor_MQTTa, numero_puerto_MQTT);
        client.setCallback(callback);

        pinMode(Entrada_configuracion, INPUT_PULLUP);
        pinMode(Entradas[0], INPUT_PULLUP);
        pinMode(Entradas[1], INPUT_PULLUP);
        pinMode(Salida[0], OUTPUT);
        pinMode(Salida[1], OUTPUT);

        // attachInterrupt(digitalPinToInterrupt(Entradas[0]), E0_interr, FALLING);
        // attachInterrupt(digitalPinToInterrupt(Entradas[1]), E1_interr, FALLING);

        servidorhttp();
        proximoEstado();
    }
}

void comms()
{
    temp_Tcomms = now;

    for (int i = 0; i <= 4; i++)
    {
        Serial.println(nombre_completo_temporizadores[i]);
    }
    for (int i = 0; i <= 2; i++)
    {
        Serial.println(nombre_completo_ordenes[i]);
    }
    for (int i = 0; i <= 2; i++)
    {
        Serial.println(nombre_completo_entradas[i]);
    }
    Serial.println("Inicio Reporte");
    Serial.println(nombre_estado);
    Serial.println((String)T1);
    Serial.println((String)T2);
    Serial.println((String)T3);
    Serial.println((String)Tapertura);
    Serial.println((String)Tcierre);

    unsigned long t1 = T1/segundos;
    unsigned long t2 = T2/segundos;
    unsigned long t3 = T3/minutos;
    unsigned long tapertura = Tapertura/segundos;
    unsigned long tcierre = Tcierre/segundos;
    
    String T1a=(String)t1;
    String T2a=(String)t2;
    String T3a=(String)t3;
    String Taperturaa=(String)tapertura;
    String Tcierrea=(String)tcierre;
    

    sa = nombre_estado + "c";
    String a="";
    String c[2]={"",""};
    String d[2]={"",""};

    for (int i = 0; i <= 4; i++)
    {
        sb[i] = nombre_completo_temporizadores[i]+ "c";
    }

    for (int i = 0; i <= 1; i++)
    {
        sc[i] = nombre_completo_entradas[i]+ "c";
        c[i]=(String)digitalRead(Entradas[i]);
    }

    for (int i = 0; i <= 1; i++)
    {
        sd[i] = nombre_completo_ordenes[i]+ "c";
        d[i]=(String)ordenes[i];

    }
    
    switch (estado) {
        case 0:
            a="Abriendo señal activa";
        break;
        case 1:
            a="Cerrando señal activa";
        break;
        case 2:
            a="Abierta";
        break;
        case 3:
            a="Cerrada";
        break;
        case 4:
            a="Abierta con deteccion";
        break;
        case 5:
            a="Abriendo con deteccion";
        break;
        case 6:
            a="Configuracion";
        break;
        case 7:
            a="Abriendo";
        break;
        case 8:
            a="Cerrando";
        break;
        case 9:
            a="Abriendo con deteccion y señal activa";
        break;
    }

    client.publish(sb[0].c_str(), Taperturaa.c_str());
    client.publish(sb[1].c_str(), Tcierrea.c_str());
    client.publish(sb[2].c_str(), T1a.c_str());
    client.publish(sb[3].c_str(), T2a.c_str());
    client.publish(sb[4].c_str(), T3a.c_str());
    client.publish(sc[0].c_str(), c[0].c_str());
    client.publish(sc[1].c_str(), c[1].c_str());
    client.publish(sd[0].c_str(), d[0].c_str());
    client.publish(sd[1].c_str(), d[1].c_str());
    client.publish(sa.c_str(), a.c_str());

}

void loop()
{
    Valor_entrada_configuracion = digitalRead(Entrada_configuracion);
    entradas[0] = digitalRead(Entradas[0]);
    entradas[1] = digitalRead(Entradas[1]);

    if (Valor_entrada_configuracion == true)
    {
        Serial.println("En Configuración");
        delay(1000);
    }
    else
    {
        if (!client.connected())
        {
            reconnect();
        }
        client.loop();

        now = millis();

        proximoEstado();

        if (now - temp_Tcomms > Tcomms) // Lazo envío estado
        {
            comms();
        }
                
        if (cambio_estado)
        {
            switch (estado)
            {
                case Estados::Cerrando_señal_activa:
                {
                    Serial.println("Estado Cerrando_señal_activa");
                    digitalWrite(Salida[0], HIGH);
                    digitalWrite(Salida[1], LOW);
                    ordenes[0] = false;                    
                    ordenes[1] = false;                    
                    entradas[0]=false;
                    entradas[1]=false;
                    cambio_estado=false;
                }
                break;
                case Estados::Cerrando:
                {
                    Serial.println("Estado Cerrando");
                    digitalWrite(Salida[0], LOW);
                    digitalWrite(Salida[1], LOW);
                    ordenes[0] = false;                    
                    ordenes[1] = false;                    
                    entradas[0]=false;
                    entradas[1]=false;
                    cambio_estado=false;
                }
                break;
                case Estados::Cerrada:
                {
                    Serial.println("Estado Cerrada");
                    digitalWrite(Salida[0], LOW);
                    digitalWrite(Salida[1], LOW);
                    ordenes[0] = false;                    
                    ordenes[1] = false;                    
                    entradas[0]=false;
                    entradas[1]=false;
                    cambio_estado=false;
                }
                break;
                case Estados::Abriendo_señal_activa:
                {
                    Serial.println("Estado Abriendo_señal_activa");
                    digitalWrite(Salida[0], LOW);
                    digitalWrite(Salida[1], HIGH);
                    ordenes[0] = false;                    
                    ordenes[1] = false;                    
                    entradas[0]=false;
                    entradas[1]=false;
                    cambio_estado=false;
                }
                break;
                case Estados::Abriendo:
                {
                    Serial.println("Estado Abriendo");
                    digitalWrite(Salida[0], LOW);
                    digitalWrite(Salida[1], LOW);
                    ordenes[0] = false;                    
                    ordenes[1] = false;                    
                    entradas[0]=false;
                    entradas[1]=false;
                    cambio_estado=false;
                }
                break;
                case Estados::Abierta:
                {
                    Serial.println("Estado Abierta");
                    digitalWrite(Salida[0], LOW);
                    digitalWrite(Salida[1], LOW);
                    ordenes[0] = false;                    
                    ordenes[1] = false;                    
                    entradas[0]=false;
                    entradas[1]=false;
                    cambio_estado=false;
                }
                break;
                case Estados::Abierta_con_deteccion:
                {
                    Serial.println("Estado Abierta_con_deteccion");
                    digitalWrite(Salida[0], LOW);
                    digitalWrite(Salida[1], LOW);
                    ordenes[0] = false;                    
                    ordenes[1] = false;                    
                    entradas[0]=false;
                    entradas[1]=false;
                    cambio_estado=false;
                }
                break;
                case Estados::Abriendo_con_deteccion:
                {
                    Serial.println("Estado Abriendo_con_deteccion");
                    digitalWrite(Salida[0], LOW);
                    digitalWrite(Salida[1], LOW);
                    ordenes[0] = false;                    
                    ordenes[1] = false;                    
                    entradas[0]=false;
                    entradas[1]=false;
                    cambio_estado=false;
                }
                break;
                case Estados::Abriendo_con_deteccion_y_orden:
                {
                    Serial.println("Estado Abriendo_con_deteccion_y_orden");
                    digitalWrite(Salida[0], LOW);
                    digitalWrite(Salida[1], HIGH);
                    ordenes[0] = false;                    
                    ordenes[1] = false;                    
                    entradas[0]=false;
                    entradas[1]=false;
                    cambio_estado=false;
                    Temp_T4=now;
                }
                break;
                case Estados::Configuracion:
                {

                }
                break;                
            }
        }
    }
}
