/* TODO:
  * - Obtenir valors del servidor
  * - Afegir opció de saber el temps que farà.
  * - Afeegir cèl·lula fotoelecèctrica per no regar si fa molt sol.
  * - Implementar actualitzacions del sketch via OTA (Sembla que no pot ser, <64kb)
*/

#include <SPI.h>
#include <WiFiNINA.h>

#include "arduino_secrets.h" 

////// Variables servidor //////
// Nivells humitat: (humidityLevel)
// 600: Molt Baixa
// 525: Bastant baix
// 450: Normal
// 375: Bastant alt
// 300: Molt alta
// -----
// Frecuència de test dels sensors: (freq)
// 1: Molt Alta -> Cada 60 segons
// 15: Alta -----> Cada 15 minuts
// 30: Mitjana --> Cada 30 minuts
// 45: Baixa ----> Cada 45 minuts
// 60: Molt Baixa -> Cada hora.

const int pumpRelay[4] = { 2, 3, 4, 5 };
const int moistureSensor[4] = { A0, A1, A2, A3 };
const int ON = LOW;
const int OFF = HIGH;

//Més baix = més humit. 
//275 -> Molt humit. 
//600 -> completament sec.
int humidityLevel = 450;
int nivellHumitat[4] = { 450, 450, 450, 450 }; // TODO: Aquí tindria que agafar els nivells d'umitat marcats al servidor.
int moistureThreshold[4] = { nivellHumitat[0], nivellHumitat[1], nivellHumitat[2], nivellHumitat[3] };  // Ajustar a les necesitats de cada planta
int moistureLevel;  


int freq = 1;                           // TODO: Aquí tindria que agafar el valor del servidor
long retestHumidityTime = 60000 * freq;  // Cada quanta estona es fa un test d'humitat

// Paràmetres de la wifi
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;


void setup() {
  //Inicialitza la consola
  Serial.begin(9600);

  //Inicialitza el relé i el sensor d'humitat
  for(int i = 0; i < 4; i++) {
    pinMode(pumpRelay[i], OUTPUT);
    digitalWrite(pumpRelay[i], OFF);  
    pinMode(moistureSensor[i], INPUT);
  }

  delay(500);

  //Inicialitza la Wifi *******************************************************
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Check del módul Wifi:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Comunicació amb el mòdul Wifi ha fallat");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Si us plau, actualitzeu el firmware");
  }

  // intenta connectar a la wifi:
  while (status != WL_CONNECTED) {
    Serial.print("Intentant connectar al SSID WPA2: ");
    Serial.println(ssid);
    // Connecta a una xarxa WPA/WPA2:
    status = WiFi.begin(ssid, pass);

    // espera 10 segons per la connexió:
    delay(10000);
  }

  // Ara ja estàs connectat:
  Serial.print("Connectat a la xarxa");
  // FI Wifi setup **************************************************************


  // Recollir valors d'inici del servidor robot
}



void loop() {

  for(int i = 0; i < 4; i++) {
    moistureLevel = analogRead(moistureSensor[i]);
    Serial.print("NIVELL d'HUMITAT ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(moistureLevel);
    
    // Si la terra está molt seca, engega la bomba d'aigua.
    // D'altre manera, apagala.
    if(moistureLevel > moistureThreshold[i]) {
      digitalWrite(pumpRelay[i], ON);
      retestHumidityTime = 5000; // El motor no volem que es quedi engegat sense tornar a mirar la humitat més de 5 segons.
    } else {
      digitalWrite(pumpRelay[i], OFF);
    }
   }
  Serial.println();
  delay(retestHumidityTime);
  retestHumidityTime = 60000 * freq; // Torna a posar el temps de retest de la humitat a l'establert al servidor.
}
