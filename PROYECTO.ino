//Incluimos las librerías necesarias, importando la librería UniversalTelegramBot en zip
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Introducir la red WIFI y contraseña 
const char* ssid = "Mitelefono";
const char* password = "pagamoroso04";

// Iniciamos el BOT de telegram introduciendo el token 
#define BOTtoken "8132305778:AAGYGDsYhYmfsikUZBxYmnuBStfQshX9bXc"  

// Introducimos nuestra ID chat de telegram 
#define CHAT_ID "1313718119"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

//Introducimos las variables necesarias para el funcionamiento del código, tanto temas de patillaje como estados.
const int buzPin = 25; //Definimos el buzzer
const int led = 32; //Definimos el LED
const int PIR = 27; // Sensor PIR
bool motionDetected = false;
bool alarmaActiva = false;    // Controlamos el estado de la alarma
bool sensorActivo = false;    // Variable para controlar si el sensor está activo o no

// Variables de retardo para la verificación del movimiento
unsigned long lastDetectionTime = 0;  // Tiempo de la última detección
const unsigned long detectionDelay = 100;  // Retardo de verificación en milisegundos

// Variable para control del tiempo del último mensaje enviado, lo utilizaremos para que no mande continuamente mensajes
unsigned long lastMessageTime = 0; // Tiempo del último mensaje de movimiento enviado
const unsigned long messageInterval = 30000; // Intervalo mínimo entre mensajes en milisegundos (30 segundos)

// Indica cuando se detecta movimiento
void IRAM_ATTR detectsMovement() {
  // Solo detecta si el sensor está activo y ha pasado el retardo de tiempo agregado arriba en las variables
  if (sensorActivo && (millis() - lastDetectionTime > detectionDelay)) {
    motionDetected = true;
    lastDetectionTime = millis();
  }
}


//Módulo que se encarga de enviar mensajes dependiendo de la situación en la que nos encontremos
void handleNewMessages(int numNewMessages) {
  //Bucle for que hace de intermediario entre telegram(los comandos que indicamos) y nuestro código actual
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    
    //Verificamos que el ID de usuario de telegram sea correcto con el movil que tenemos, de lo contrario el acceso será denegado
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "ACCESO DENEGADO", "");
      continue;
    }

    String text = bot.messages[i].text;

    //Este if hace activar la alarma con un pitido y un led 
    if (text == "/alarma_on") {
      bot.sendMessage(chat_id, "Alarma activada", "");
      alarmaActiva = true;               // Activa la alarma
      digitalWrite(buzPin, HIGH);         // Enciende el buzzer
      digitalWrite(led, HIGH); 
    }

    //Apagamos la alarma
    if (text == "/alarma_off") {
      bot.sendMessage(chat_id, "Alarma desactivada", "");
      alarmaActiva = false;              // Desactiva la alarma
      digitalWrite(buzPin, LOW);         // Apaga el buzzer
      digitalWrite(led, LOW);            // Apaga el LED
    }


    if (text == "/activar") {
      bot.sendMessage(chat_id, "Sensor activado", "");
      sensorActivo = true;               // Activa el sensor PIR
      attachInterrupt(digitalPinToInterrupt(PIR), detectsMovement, RISING); // Reactiva el sensor
    }

    //Desactivamos el sensor de manera permanente hasta que no se indique lo contrario con /activar
    if (text == "/desactivar") {
      bot.sendMessage(chat_id, "Sensor desactivado", "");
      sensorActivo = false;              // Desactiva el sensor PIR
      detachInterrupt(digitalPinToInterrupt(PIR)); // Desactiva el sensor
    }
  }
}

void setup() {
  Serial.begin(9600);

  // Configuración del sensor PIR, buzzer y led
  pinMode(PIR, INPUT_PULLUP);
  pinMode(buzPin, OUTPUT);
  pinMode(led, OUTPUT);

  // Conexión WiFi, NECESARIO LAS BIBLIOTECAS
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Agrega el certificado raíz para api.telegram.org
  
  //Verificamos que la conexión al WIFI se haya realizado correctamente, de lo contrario escribirá "." continuamente
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  //Indicamos la conexión correcta del WIFI
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //Nada más iniciar el bot, mandará un mensaje de bienvenida con los comandos de los que dispone el usuario
  String welcomeMessage = "Bot iniciado\n\n";
  welcomeMessage += "Comandos disponibles:\n";
  welcomeMessage += "/alarma_on - Activa la alarma y el buzzer\n";
  welcomeMessage += "/alarma_off - Desactiva la alarma y el buzzer\n";
  welcomeMessage += "/activar - Activa el sensor de movimiento\n";
  welcomeMessage += "/desactivar - Desactiva el sensor de movimiento\n";
  bot.sendMessage(CHAT_ID, welcomeMessage, "");
}

void loop() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  //Bucle que está continuamente detectando la entrada de nuevos mensajes
  while (numNewMessages) {
    handleNewMessages(numNewMessages);
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
  
  //Verificamos el movimiento
  if (motionDetected) {
    unsigned long currentMillis = millis();
    // Solo envía el mensaje si han pasado al menos 30 segundos desde el último mensaje, esto hará que el usuario no reciba muchos mensajes
    if (currentMillis - lastMessageTime >= messageInterval) {
      bot.sendMessage(CHAT_ID, "Movimiento detectado!!", "");
      lastMessageTime = currentMillis; // Actualiza el tiempo del último mensaje
    }
    motionDetected = false; // Reinicia la variable de movimiento detectado
    Serial.println("Movimiento detectado"); 
  } else {
    Serial.println("No se detecta movimiento"); //Indicamos que no se detecta movimiento por el monitor serie
  }
}



