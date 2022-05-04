/* TODO:
  * - Afegir opció de saber el temps que farà. Per tal de poder predir si fa falta regar o en unes hores es posarà a ploure.
  * - Afeegir cèl·lula fotoelecèctrica per no regar si fa molt sol. (Mirar si la puc situar bé).
  * - La implamentació OTA no es pot realitzar ja que la memòria flash del dispositiu és massa petita. Mínim 64kb.
  * - Gestió d'Streams no implementada, ja que el mòdul Wifi només permet fer-ho d'un en un. Intentar fer-ho més endavant.
*/

////// Variables servidor //////
// Nivells humitat: (humidityLevel)
// 600: Molt Baixa
// 525: Bastant baix
// 450: Normal
// 375: Bastant alt
// 300: Molt alta
// Nivells del sensor concret
// Més baix = més humit. 
// 275 -> Molt humit. 
// 600 -> completament sec.
// -----
// Frecuència de test dels sensors: (freq)
// 1: Molt Alta -> Cada 60 segons
// 15: Alta -----> Cada 15 minuts
// 30: Mitjana --> Cada 30 minuts
// 45: Baixa ----> Cada 45 minuts
// 60: Molt Baixa -> Cada hora.
// -----
// Previsió meteorològica: (weather).

#include <SPI.h>
#include "Firebase_Arduino_WiFiNINA.h"
#include "arduino_secrets.h"

// El path és important per llegir i enviar la informació a firebase. També creem un objecte de Firebase.
String path="/torretes";
FirebaseData fbdo;

//Paràmetres de la wifi
char ssidf[] = SECRET_SSIDF;
char passf[] = SECRET_PASSF;
char dbsf[] = SECRET_DBSF;
char fbhost[] = SECRET_FBHOST;

// Variables per el temps que porta executant-se el programa i el temps des de la última execució de les medicions.
unsigned long timeActual = 0;
unsigned long timeLastExecute = 0;

//Inicialitza la variable sense valor. Ja agafa el valor del servidor firebase.
int freq;

// Inicialitza paràmetres del relé i els sensors.
const int pumpRelay[4] = { 2, 3, 4, 5 };
const int moistureSensor[4] = { A0, A1, A2, A3 };
const int ON = LOW;
const int OFF = HIGH;

// Inicialitza la variable sense valor per posteriorment recollira-la del servidor fb
int nivellHumitat[4];
int moistureLevelSensor[4];  


// ********************************************************
// *            Inici de les funcions                     *
// ********************************************************

// Funció per inicialitzar relé i sensor d'humitat.
void initialize_waterPump() {
  //Inicialitza el relé i el sensor d'humitat
  for(int i = 0; i < 4; i++) {
    pinMode(pumpRelay[i], OUTPUT);
    digitalWrite(pumpRelay[i], OFF);  
    pinMode(moistureSensor[i], INPUT);
  }
  delay(500);
}

// Funció que activa el relé i comença a regar.
void activateRelay(int i) {
  digitalWrite(pumpRelay[i], ON);
}

// Funció que desactiva el relé i deixa de regar.
void deactivateRelay(int i) {
  digitalWrite(pumpRelay[i], OFF);
}

// Comprova si els nivells d'humitat són els adecuats, de no ser així activa/desactiva el relé.
void testMoistureLevel() {
  for(int i = 0; i < 4; i++) {
    moistureLevelSensor[i] = analogRead(moistureSensor[i]);
    sendData(i,moistureLevelSensor[i]); // Envia les dades a la bbdd firebase. Concretament
    if(moistureLevelSensor[i] > nivellHumitat[i]) {
      activateRelay(i);
    } else {
      deactivateRelay(i);
    }
  }
}

// Aquest funció serveix per que si hi ha un relé obert (és a dir, està regant), no faci el següent test al cap de 5 segons i no el temps establert per servidor.
// De no fer-ho es podria donar el cas que un relé estigués fins a 60 minuts funcionant.
bool checkOpenRelay() {
  for(int i = 0; i < 4; i++) {
    if(digitalRead(pumpRelay[i]) == ON) {
      return true;
    } else {
      return false;
    }
  }
}

// Aquesta funció agafa els valors de les variables del servidor que posteriorment s'inicialitzaran al setup() i es recomprova segons la frequencia.
void getallServerOptions() {
  
  // Recupera la freqüencia de refresc dels sensors
  freq = getdataFreq();

  // Això és una mica ticky. Es tracta de recollir el punter retornat per la funció getdataNivellHumitat per poder treballar amb l'array
  int *hlptr;
  hlptr = getdataNivellHumitat();
  for(int i = 0; i < 4; i++) {
    nivellHumitat[i] = *(hlptr + i);
  }
}

long unsigned humidityTime() {
  return 60000 * freq; 
}

// La següent funció inicialitza els paràmetres del servidor firebase.
void initialize_wifi_firebase() {
  Serial.begin(115200);
  delay(100);
  
  //Connecta a la Wifi
  Serial.print("Connectant a la Wi-Fi");
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED)
  {
    status = WiFi.begin(ssidf, passf);
    Serial.print(".");
    delay(100);
  }
  Serial.println();
  Serial.print("Connectat amb IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  //Dades d'autentificació
  Firebase.begin(fbhost, dbsf, ssidf, passf);
  Firebase.reconnectWiFi(true);
}

void showError() {
  Serial.println("FAILED");
  Serial.println("REASON: " + fbdo.errorReason());
  Serial.println("=================");
  Serial.println();
}

/* Estructura de la bbdd de firebase 
 *
 *
 * /Torretes
 *    Sensors humitat
 *      Torreta 1
 *        Sensor 0 (int valor)
 *        Sensor 1 (int valor)
 *      Torreta 2
 *        Sensor 2 (int valor)
 *        Sensor 3 (int valor)
 *    Noms plantes
 *      Torreta 1
 *        Nom planta 0 (String valor)
 *        Nom planta 1 (String valor)
 *      Torreta 2
 *        Nom planta 2 (String valor)
 *        Nom planta 3 (String valor)
 *    Freqüencia de refresc (int 1 - 15 - 30 - 45 - 60 minuts) 
 *    Nivells d'humitat (int 600 - 525 - 450 - 375 - 300)
 *    weather (bool) (PENDENT IMPLEMENTAR)
 *      
 *   
*/

// Aquesta funció envia les dades dels sensor d'humitat al servidor firebase. La primera part detecta la torreta per després poder treballar amb app mobil.
void sendData(int i, int valor) {
  int torreta;
  if (i<2) {
    torreta = 1;
  } else {
    torreta = 2;
  }
  if (!Firebase.setInt(fbdo, path + "/Sensor_Humitat/" + torreta + "/" + i, valor)) {
    showError();
  }
}

// Aquesta funció retorna les dades de frequencia configurada al servidor.
int getdataFreq() {
  if (Firebase.getInt(fbdo, path + "/frecuencia")) {
    int fq = fbdo.intData();
    fbdo.clear();
    return fq;
  } else {
    showError();
  }
}

// Aquesta funció retorna els valors configurats de nivell d'humitat. 
// La funció retorna un punter, ja que interessa recuperar les dades com un array.
int * getdataNivellHumitat() {
  static int hl[4];
  for (int i = 0; i < 4; i++) {
    if (Firebase.getInt(fbdo, path + "/Config_Humidity/" + i)) {
      hl[i] = fbdo.intData();
    } else {
      showError();
    }
  }
  fbdo.clear();
  return hl;
}


// *********************************************************
// ********************** setup() **************************
// *********************************************************

void setup() {
  //Inicialitza la Wifi i firebase
  initialize_wifi_firebase();

  //Inicialitza el relé i el sensor d'humitat
  initialize_waterPump();
}

// *********************************************************
// ********************** loop() ***************************
// *********************************************************

void loop() {
  timeActual = millis();
  if (timeActual > (timeLastExecute + humidityTime()) || timeLastExecute == 0 || checkOpenRelay()) {
    getallServerOptions();
    timeLastExecute = millis();
    testMoistureLevel();
  }
}
