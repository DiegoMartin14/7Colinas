#include <Arduino.h>
#include <WiFiManager.h>
#include <FirebaseESP32.h>
#include <HTTPClient.h>

int configWifi = 34; // entrada digital para configurar wifi
int sensor1 = 36;    // entrada anañogica para el sensor de temperatura del tanque 1
float rango = 10.24; //
float dividirSensor;
float temperaturaTanque1;
char caracter = 'f';
int estado = 0;
const char *FIREBASE_PROJECT_ID = "***"; // Identificador de tu proyecto
const char *FIREBASE_API_KEY = "****";    // Tu clave API
const char *USER_EMAIL = "****";          // Correo electrónico del usuario
const char *USER_PASSWORD = "***";       // Contraseña del usuario

FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

void setup()
{
    Serial.begin(115200);

    // WiFiManager, inicialización local. Una vez que ha terminado su trabajo, no es necesario mantenerlo
    WiFiManager wm;

    // restablecer la configuración - borrar las credenciales almacenadas para pruebas
    // estas son almacenadas por la biblioteca esp
    // wm.resetSettings();

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
        Serial.println("Failed to connect"); // Falló la conexión
        // ESP.restart();
    }
    else
    {
        // si llegas aquí, te has conectado al WiFi
        Serial.println("connected...yeey :)"); // conectado... ¡yay! :)
    }

    /*  // Configura las credenciales de Firebase
      config.api_key = FIREBASE_API_KEY;
      auth.user.email = USER_EMAIL;
      auth.user.password = USER_PASSWORD;

      // Inicializa Firebase
      Firebase.begin(&config, &auth);
      //Firebase.reconnectWiFi(true);

      // Espera hasta que se conecte a Firebase
      Serial.print("Conectando a Firebase");
      while (!Firebase.ready())
      {
          delay(1000);
          Serial.print(".");
      }
      Serial.println();
      Serial.println("Conexión a Firebase establecida");
  */
    // Configuración de los pines de entrada analógica
    analogReadResolution(10);       // Resolución de 10 bits
    analogSetAttenuation(ADC_11db); // Tensión de referencia de 3.3V
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(configWifi, INPUT_PULLDOWN);
    pinMode(sensor1, INPUT);
}

void loop()
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);

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
            Serial.println("Failed to connect"); // Falló la conexión
            // ESP.restart();
        }
        else
        {
            // si llegas aquí, te has conectado al WiFi
            Serial.println("connected...yeey :)"); // conectado... ¡yay! :)
        }
    }

    if (Serial.available() > 0)
    {                             // Verificar si hay datos disponibles para leer
        caracter = Serial.read(); // Leer un caracter desde el monitor serie
    }

    if (caracter == 't')
    {
        Serial.print("Temperatura del tanque 1: ");
        Serial.println(temperaturaTanque1);
        caracter = 'f';

        /*   // Construye el objeto JSON
           FirebaseJson json;
           json.set("timestamp", millis());
           json.set("temperature", temperaturaTanque1);

           // Envía los datos a una nueva colección y documento en Firestore
           String documentPath = "sensores/temperatura_" + String(millis()); // Cambia esto a tu colección y documento
           if (Firebase.Firestore.createDocument(&firebaseData, FIREBASE_PROJECT_ID, "", documentPath, json.raw()))
           {
               Serial.println("Datos enviados exitosamente a Firestore");
           }
           else
           {
               Serial.println("Error al enviar datos a Firestore");
               Serial.println(firebaseData.errorReason());
           }*/
    }
    dividirSensor = analogRead(sensor1);
    temperaturaTanque1 = dividirSensor / rango;

    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
}
