#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP32Time.h>
#include <secrets.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>
// #include <FirebaseClient.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <map>
#include <AsyncWebSocket.h>

// Definicion de variables
double temperatura;
double tanque;
unsigned long previousMillis = 0;
unsigned long documentPreviousMillis = 0;
const unsigned int documentCreationInterval = 3000;
String fecha;
String hora;
int currentTank = 1;

// Firebase variables
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

////////////////7 comienzo la prueba del server ///////////////////////////////77
AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");
struct AlarmSettings
{
    float minTemp;
    float maxTemp;
};

// Memory to store alarm settings for each tank
AlarmSettings alarmSettings[6];

void onRequest(AsyncWebServerRequest *request)
{
    // Handle Unknown Request
    request->send(404, "text/plain", "Not found");
}

// Función para manejar solicitudes HTTP para configurar valores de alarma
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->opcode == WS_TEXT)
        {
            data[len] = 0;
            String message = (char *)data;
            if (message == "getData")
            {
                // Obtener datos de Firestore y enviarlos al cliente
                String path = "/Producción/Tanque" + String(currentTank);
                if (Firebase.getJSON(fbdo, path.c_str()))
                {
                    if (fbdo.dataType() == "json")
                    {
                        String jsonString;
                        fbdo.jsonObject().toString(jsonString, true);
                        client->text(jsonString);
                    }
                    else
                    {
                        client->text("Error retrieving data");
                    }
                }
                else
                {
                    client->text(fbdo.errorReason());
                }
            }
        }
    }
}

// Función para obtener los datos del tanque
void handleGetTankData(AsyncWebServerRequest *request)
{
    if (request->hasParam("tank"))
    {
        int tankId = request->getParam("tank")->value().toInt();
        // Aquí obtienes los datos del tanque desde Firestore o alguna variable
        String temperature = String(temperatura); // Ejemplo, reemplaza con el valor real
        String date = fecha;                      // Ejemplo, reemplaza con el valor real
        String time = hora;                       // Ejemplo, reemplaza con el valor real

        String jsonResponse = "{\"temperature\": \"" + temperature + "\", \"date\": \"" + date + "\", \"time\": \"" + time + "\"}";
        request->send(200, "application/json", jsonResponse);
    }
    else
    {
        request->send(400, "text/plain", "Bad Request");
    }
}

// Función para configurar la alarma de temperatura
void handleSetAlarm(AsyncWebServerRequest *request)
{
    if (request->hasParam("tank", true) && request->hasParam("minTemp", true) && request->hasParam("maxTemp", true))
    {
        int tank = request->getParam("tank", true)->value().toInt();
        float minTemp = request->getParam("minTemp", true)->value().toFloat();
        float maxTemp = request->getParam("maxTemp", true)->value().toFloat();

        // Update the local alarm settings
        alarmSettings[tank - 1].minTemp = minTemp;
        alarmSettings[tank - 1].maxTemp = maxTemp;

        // Send response
        request->send(200, "text/plain", "Alarm settings updated");
    }
    else
    {
        request->send(400, "text/plain", "Bad Request");
    }
}

// Función para obtener la configuración de alarma de un tanque
void handleGetAlarmSettings(AsyncWebServerRequest *request)
{
    if (request->hasParam("tank"))
    {
        int tank = request->getParam("tank")->value().toInt();
        float minTemp = alarmSettings[tank - 1].minTemp;
        float maxTemp = alarmSettings[tank - 1].maxTemp;

        // Create JSON response
        String jsonResponse;
        JsonDocument doc;
        doc["minTemp"] = minTemp;
        doc["maxTemp"] = maxTemp;
        serializeJson(doc, jsonResponse);

        request->send(200, "application/json", jsonResponse);
    }
    else
    {
        request->send(400, "text/plain", "Bad Request");
    }
}

// Función para reiniciar el ESP32
void handleReset(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", "Reiniciando ESP32...");
    ESP.restart();
}

//////////////// termino la prueba del server //////////////////////////////////

// con esto obtengo la fecha y hora de un servidor NTP
ESP32Time rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", -10800, 60000); // Servidor del Observatorio Naval Buenos Aires UTC-3 (Argentina)

void setup()
{
    Serial.begin(115200);

    // WiFiManager
    WiFiManager wm;
    bool res;
    res = wm.autoConnect("Microcontrolador");
    if (!res)
    {
        Serial.println("Falló la conexion a wifi"); // Falló la conexión
        ESP.restart();
    }
    else
    {
        // si llegas aquí, te has conectado al WiFi
        Serial.println("Conectado a wifi:)"); // conectado... ¡yay! :)
    }

    // Inicializar SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("Error al montar SPIFFS");
        return;
    }
    Serial.println("SPIFFS montado correctamente");

    // configuracion para horario
    timeClient.begin();
    configTime(-10800, 0, "europe.pool.ntp.org");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        rtc.setTimeStruct(timeinfo);
    }

    // Configuracion de Firebase
    config.api_key = API_KEY;
    auth.user.email = USUARIO_EMAIL;
    auth.user.password = USUARIO_CONTRA;
    config.database_url = DATABASE_URL;
    
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    Serial.println("CONECTADO ");

    // Initialize WebSocket
    webSocket.onEvent(onWsEvent);
    server.addHandler(&webSocket);

    // Route for setting alarm values
    server.on("/setAlarm", HTTP_POST, handleSetAlarm);

    // Route for getting alarm values
    server.on("/getAlarmSettings", HTTP_GET, handleGetAlarmSettings);

    // Serve static files
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // Start the server
    server.begin();
}

void loop()
{
    webSocket.cleanupClients();
    Firebase.ready();

    // muestro fecha y hora actual
    timeClient.update();
    Serial.print("Hora: ");
    Serial.println(timeClient.getFormattedTime()); // Muestra hora en terminal
    Serial.print("Fecha: ");
    Serial.println(rtc.getDate()); // Muestra fecha en terminal
    delay(1000);

    // Verifica si la aplicacion esta lista para usar
    if (millis() - documentPreviousMillis > documentCreationInterval)
    {
        documentPreviousMillis = millis();

        String documentPath = "/Producción/" + String(timeClient.getEpochTime()); // Crea una coleccion llamada Produccion con un documento random en firebase

        temperatura = ++temperatura; // esto se reemplaza por las mediciones reales de los tanques
        tanque = ++tanque;           // esto igual

        fecha = rtc.getDate();
        hora = timeClient.getFormattedTime();

        json.clear();
        json.add("temperatura", temperatura);
        json.add("tanque", tanque);
        json.add("fecha", fecha);
        json.add("hora", hora);

        if (Firebase.pushJSON(fbdo, documentPath.c_str(), json))
        {
            Serial.println("Datos enviados a Firebase");
        }
        else
        {
            Serial.println("Error al enviar datos: " + fbdo.errorReason());
        }
    }
}