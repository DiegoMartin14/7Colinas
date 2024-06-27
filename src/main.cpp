#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP32Time.h>
#include <secrets.h>

// Definicion de variables
int configWifi = 34; // entrada digital para configurar wifi
int estado = 0;
double temperatura;
double tanque;
unsigned long previousMillis = 0;
unsigned long documentPreviousMillis = 0;
const unsigned int documentCreationInterval = 3000;
String fecha;
String hora;
// con esto obtengo la fecha y hora de un servidor NTP
ESP32Time rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", -10800, 60000); // Servidor del Observatorio Naval Buenos Aires UTC-3 (Argentina)

// DE ACA PARA ABAJO ESTA LO QUE TIENE QUE VER CON FIRESTORE
UserAuth user_auth(API_KEY, USUARIO_EMAIL, USUARIO_CONTRA, 3000);

void asyncCB(AsyncResult &aResult);

void printResult(AsyncResult &aResult);

DefaultNetwork network; // Inicializar con un parámetro booleano para habilitar/deshabilitar la reconexión de red

FirebaseApp app;

WiFiClientSecure ssl_client; // es una clase que proporciona una conexión segura (usando SSL/TLS) a través de WiFi.

using AsyncClient = AsyncClientClass; // Cambia de nombre

AsyncClient aClient(ssl_client, getNetwork(network));

Firestore::Documents Docs; // es parte de la plataforma Firebase y se utiliza para interactuar con la base de datos Firestore.

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

    // Configuracion de Firebase
    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    Serial.println("Initializing app...");

#if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040) // Verifica si alguna de esas macros está definida
    ssl_client.setInsecure();                                  // llama a una función llamada setInsecure() en un objeto llamado ssl_client
#endif

    initializeApp(aClient, app, getAuth(user_auth), asyncCB, "authTask"); // Se usa para inicializar una aplicación de Firebase con la configuración proporcionada

    app.getApp<Firestore::Documents>(Docs);

    // configuracion para horario
    timeClient.begin();
    configTime(-10800, 0, "europe.pool.ntp.org");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        rtc.setTimeStruct(timeinfo);
    }

    // Configuración de los pines
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(configWifi, INPUT_PULLDOWN);
}

void loop()
{
    app.loop();
    Docs.loop();
    // muestro fecha y hora actual
    timeClient.update();
    Serial.print("Hora: ");
    Serial.println(timeClient.getFormattedTime()); // Muestra hora en terminal
    Serial.print("Fecha: ");
    Serial.println(rtc.getDate()); // Muestra fecha en terminal
    delay(1000);

    // Verifica si la aplicacion esta lista para usar
    if (app.ready() && documentPreviousMillis < millis() - documentCreationInterval) // Verifica si ya pasaron 3 segundos de la ultima vez que se cargaron datos en la base
    {
        documentPreviousMillis = millis();

        String documentPath = "Produccion/" + String(timeClient.getFormattedTime()); // Crea una coleccion llamada Produccion con un documento random en firebase

        temperatura = ++temperatura; //esto se reemplaza por las mediciones reales de los tanques
        tanque = ++tanque; //esto igual

        fecha = (rtc.getDate());
        hora = (timeClient.getFormattedTime());

        Values::DoubleValue temperaturaValue(temperatura);
        Values::DoubleValue tanqueValue(tanque);
        Values::StringValue fechaValue(fecha);
        Values::StringValue horaValue(hora);

        Document<Values::Value>
            doc("temperatura", Values::Value(temperaturaValue)); // Crea una coleccion llamada Temperatura con el valor que toma de la medicion
        doc.add("Tanque", Values::Value(tanqueValue));           // Crea una coleccion llamada Tanque con el valor del numero de tanque al que se refiere la medicion
        doc.add("Fecha", Values::Value(fechaValue));             // Crea una coleccion llamada Fecha con la fecha actual
        doc.add("Hora", Values::Value(horaValue));
        Docs.createDocument(aClient, Firestore::Parent(FIREBASE_PROYECTO_ID), documentPath, DocumentMask(), doc, asyncCB, "Documento creado  \n");
    }

    // Configurar una nueva red presionando un boton
    estado = digitalRead(configWifi);
    if (estado == 1)
    {
        estado = 0;
        // WiFiManager, inicialización local. Una vez que ha terminado su trabajo, no es necesario mantenerlo
        WiFiManager wm;

        // restablecer la configuración - borrar las credenciales almacenadas para pruebas
        // estas son almacenadas por la biblioteca esp
        wm.resetSettings();

        // Conectar automáticamente usando las credenciales guardadas,
        // si la conexión falla, inicia un punto de acceso con el nombre especificado ("AutoConnectAP"),
        // si está vacío, generará automáticamente el SSID, si la contraseña está en blanco será un AP anónimo (wm.autoConnect())
        // luego entra en un bucle de espera de configuración y devolverá el resultado de éxito

        bool res;
        // res = wm.autoConnect(); // nombre de AP generado automáticamente a partir del chipid
        // res = wm.autoConnect("AutoConnectAP"); // AP anónimo
        res = wm.autoConnect("Microcontrolador"); // AP protegido con contraseña

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
    }
}

void asyncCB(AsyncResult &aResult)
{
    printResult(aResult);
}

void printResult(AsyncResult &aResult)
{
    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.available())
    {
        Firebase.printf("task: %s", aResult.uid().c_str());
        //, payload : % s\n ", aResult.uid().c_str(), aResult.c_str());
    }
}