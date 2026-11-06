#include <WiFi.h>
#include <PubSubClient.h>

// ====================================================================
// PROYECTO IOT - MONITOREO AMBIENTAL DE LUMINOSIDAD Y AUTOMATIZACIÓN
// Semana 13 - Integración ESP32 + Adafruit IO + IFTTT
// Estudiante: Cesar Casado (casado_18)
// ====================================================================

// 1. Credenciales de Red WiFi
const char* ssid     = "SIMOA";
const char* password = "Afranio13";

// 2. Credenciales y Servidor Adafruit IO MQTT
#define IO_SERVER   "io.adafruit.com"
#define IO_PORT     1883
#define IO_USERNAME "casado_18"
#define IO_KEY      "aio_dSJm51SrCvc31Xf0Yl4BUIWcNvVq"

// Tópico MQTT para publicación de luminosidad
const char* mqtt_feed = "casado_18/feeds/luminosidad";

// 3. Configuración de Hardware
const int pinLDR = 34; // Pin ADC (GPIO 34) conectado al divisor de tensión con LDR

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long ultimoEnvio = 0;
// Intervalo de publicación recomendado (12 segundos para respetar límites de cuota)
const long intervalo = 12000; 

// Inicialización de la conexión WiFi
void setup_wifi() {
  Serial.println();
  Serial.println("==========================================");
  Serial.print("Conectando a red WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("[OK] WiFi Conectado exitosamente.");
  Serial.print("Dirección IP asignada: ");
  Serial.println(WiFi.localIP());
  Serial.println("==========================================");
}

// Reconexión automática al servidor MQTT de Adafruit IO
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando a Adafruit IO MQTT... ");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), IO_USERNAME, IO_KEY)) {
      Serial.println("[CONECTADO EXITOSAMENTE]");
    } else {
      Serial.print("Fallo de conexión MQTT, rc=");
      Serial.print(client.state());
      Serial.println(". Reintentando en 3 segundos...");
      delay(3000);
    }
  }
}

// Lectura analógica promediada y conversión a lux
float leerLuminosidadLux() {
  long suma = 0;
  for (int i = 0; i < 10; i++) {
    suma += analogRead(pinLDR);
    delay(10);
  }
  float promedio = suma / 10.0;

  // Mapeo lineal aproximado: 0-4095 ADC -> 0-1000 Lux
  float lux = map((int)promedio, 0, 4095, 0, 1000);
  if (lux < 0) lux = 0;
  return lux;
}

void setup() {
  Serial.begin(115200);
  pinMode(pinLDR, INPUT);

  setup_wifi();
  client.setServer(IO_SERVER, IO_PORT);
}

void loop() {
  // Mantener viva la conexión MQTT sin bloqueos
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Control de tiempo no bloqueante mediante millis()
  unsigned long ahora = millis();
  if (ahora - ultimoEnvio >= intervalo) {
    ultimoEnvio = ahora;

    float lux = leerLuminosidadLux();
    char mensaje[10];
    dtostrf(lux, 4, 1, mensaje);

    Serial.println("------------------------------------------");
    Serial.print("Luminosidad LDR (D34): ");
    Serial.print(lux);
    Serial.print(" lux  ->  ");

    if (lux < 100.0) {
      Serial.println("[ALERTA: LUZ BAJA (< 100 LUX)]");
    } else {
      Serial.println("[ESTADO NORMAL]");
    }

    if (client.publish(mqtt_feed, mensaje)) {
      Serial.println("-> Dato publicado exitosamente en Adafruit IO");
    } else {
      Serial.println("-> Error al publicar en Adafruit IO");
    }
    Serial.println("------------------------------------------\n");
  }
}
