/* TODO:
  * - Afegir opció de saber el temps que farà. Per tal de poder predir si fa falta regar o en unes hores es posarà a ploure.
  * - Afeegir cèl·lula fotoelecèctrica per no regar si fa molt sol. (Mirar si la puc situar bé).
  * - La implamentació OTA no es pot realitzar ja que la memòria flash del dispositiu és massa petita. Mínim 64kb.
  * - Gestió d'Streams no implementada, ja que el mòdul Wifi només permet fer-ho d'un en un. Intentar fer-ho més endavant.
  * - Seria interessant saber si es pot detectar el nivell de bateria per gestionar un mode sleep mitjançant alguna targeta o semblant.
*/

/* TEST:
 *  Pendent:
 *    - Test de les regles de firebase per saber si funcionen
 *    - Test de le lògica del programa, depurar-lo per saber si actua correctament. Especialment el temps entre checks si el relé està obert. Més que res
 *    s'hauria de mirar si ho fa correctament amb alguns relés oberts i altres tancats després de canvis en aquests.
 *  Fet:
 *    - Els valor dels sensors s'envien correctament a la bbdd realtime de Firebase.
 *    - Els valors de Firebase es carrguen correctament al codi com a variables.
 *    - Implementació de la BOYA del dipòsti per evitar obrir el relé i el motor si no hi ha aigua. Al final no serà una boia, si no un sensor de distància per ultra sons.
 */

 /* FET:
  * - 
  * - 
*/

////// Variables servidor //////
// Nivells humitat: (humidityLevel)
// 600: Molt Baixa
// 525: Bastant baix
// 450: Normal
// 375: Bastant alt
// 300: Molt alta
// Nivells del sensor segons datasheet
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
#include <SoftwareSerial.h>

// Aquesta variable es pel sensor de distància.
SoftwareSerial mySerial(11,10); // RX, TX
int diposit;

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

//Aquesta funció inicialitza el sensor de distància.
void initialize_distanceSensor() {
  //Inicialitza el sensor de distància
  mySerial.begin(9600);
}

/* Aquesta funció retorna la distància mesurada pel sensor.
La funció pot retirn tres tipus diferents de valors.
  *  0: El dispòsit està massa buit.
  *  1: El dispòsit està a tope.
  * -1: No s'ha pogut mesurar la distància.
  * 30 <> 400: La distància mesurada.
He triat 400, ja que no disposo de cap dipòsit superior i per tant la distància seria erronea.
Pendent de mesurar el dipòsit i canviar el 400mm per valor corresponent.
*/
int nivellDiposit() {

  unsigned char data[4]={};
  int distance;

  do {
    for (int i=0; i<4; i++) {
      data[i] = mySerial.read();
    }
  } while (mySerial.read() == 0xff);

  mySerial.flush();

  if (data[0] == 0xff) {
    int sum;
    sum = (data[0]+data[1]+data[2])&0x00FF;
    if (sum == data[3]) {
      distance = (data[1]<<8)+data[2];
      if ((distance > 30) && (distance < 400)) {
        Serial.print("distance=");
        Serial.print(distance);
        Serial.println("mm");
        delay(100);
        return distance;
      } else if (distance >= 400) {
        Serial.println("Empty tank");
        delay(100);
        return 0;
      } else {
        Serial.println("Full tank");
        delay(100);
        return 1;
      }
    } else {
      Serial.println("Checksum error");
      delay(100);
      return -1;
    }
  }
  delay(100);
  return -2;
}

// Funció per mesurar la mitjana dels valors del sensor de distància (boya).
// S'ha d'utilitzar aquesta funció ja que el sensor no funciona correctament si no es mesura de forma seguida.
// Els valors que pot retornar el sensor son:
// - 0: El dipòsit està massa buit.
// - 1: El dipòsit està a tope.
// - diancia: La distància mesurada.
// - -1: Error al mesurar la distància.
// - -2: No s'ha pogut mesurar la distància, ja que el sensor no ha facilitat cap dada.

int mitjaDiposit() {
  int suma = 0;
  int error = 0;
  int buit = 0;
  int tmp = 0;
  int count = 0;
  int ple = 0;

  for (int x = 0; x < 20; x++) {
    tmp = nivellDiposit();
    Serial.println("tmp: " + String(tmp));
    if (tmp == -1) {
      error += 1;
    } else if (tmp == 0) {
      buit += 1;
    } else if (tmp == 1) {
      ple += 1; 
    } else if (tmp == -2) {
      x -= 1;  // Augmenta el nombre de vegades que s'executa el for ja que no ha retornat valor
    } else {
      count += 1;
      suma += tmp;
    }
  }
  Serial.println("error: " + String(error));
  Serial.println("buit: " + String(buit));
  Serial.println("ple: " + String(ple));
  Serial.println("count: " + String(count));
  Serial.println("suma: " + String(suma));
  Serial.println("mitja: " + String(suma/count));
  if (error > 10) {
    return -1;
  } else if (buit > 10) {
    return 0;
  } else if (ple > 10) {
    return 1; 
  } else {
    return suma/count;
  }
}

// Funció que activa el relé i comença a regar.
void activateRelay(int i) {
  digitalWrite(pumpRelay[i], ON);
}

// Funció que desactiva el relé i deixa de regar.
void deactivateRelay(int i) {
  // Aquí potser es tindria que afegir un comprobació per saber si ja està parat
  digitalWrite(pumpRelay[i], OFF);
}

// Comprova si els nivells d'humitat són els adecuats, de no ser així activa/desactiva el relé.
void testMoistureLevel() {
  Serial.print("Lectura sensor humitat ");
  for(int i = 0; i < 4; i++) {
    Serial.print(i);
    moistureLevelSensor[i] = analogRead(moistureSensor[i]);
    Serial.println(moistureLevelSensor[i]);
    sendData(i,moistureLevelSensor[i]); // Envia les dades a la bbdd firebase. Concretament
    if(moistureLevelSensor[i] > nivellHumitat[i]) {
      Serial.print("Preparat per activar relay ");
      Serial.println(i);
      if ((diposit != -1) && (diposit != 0)) { // Si el dipòsit està buit no activa el relé
        activateRelay(i);
      } else {
        Serial.println("No s'ha pogut llegir el nivel d'aigua o el dipòsit està buit");
      }
    } else {
      Serial.print("Desactivant relay ");
      Serial.println(i);
      deactivateRelay(i);
    }
  }
}

// Aquest funció serveix per que si hi ha un relé obert (és a dir, està regant), no faci el següent test al cap de 5 segons i no el temps establert per servidor.
// De no fer-ho es podria donar el cas que un relé estigués fins a 60 minuts funcionant.
// Arreglat el return. Només retorna el true dins del bucle. El false sempre fora del bucle.
bool checkOpenRelay() {
  int status = 0;
  for(int i = 0; i < 4; i++) {
    Serial.print("RELAY ");
    Serial.print(i);
    if(digitalRead(pumpRelay[i]) == ON) {
      Serial.println("INTERRUPCIÓ DEL BUCLE RELÉ OBERT");
      return true;
    }
    Serial.println(" ..... OK");
  }
  Serial.println("NO HI HA RELÉS OBERTS");
  return false;
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

// Aquesta funció retorna el temps que ha de sumar a la última comprovació per saber si ha de tornar a fer un check dels sensor i dades del servidor.
long unsigned humidityTime() {
  Serial.print("freq = ");
  Serial.println(freq);
  return 60000 * freq; 
}

// La següent funció inicialitza els paràmetres del servidor firebase.
void initialize_wifi_firebase() {
  Serial.begin(57600);
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

// Mostra errors relacionats amb la inicialització i enviament de dades
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
 *    Nivell del dipòsit.
 *      
 *   
*/

void sendDiposit(int i) {
  if (!Firebase.setInt(fbdo, path + "/Diposit", diposit)) {
    showError();
  }
}

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

// Funció que retorna els valors configurats de nivell d'humitat. 
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

  //Inicialitza el sensor de distància
  initialize_distanceSensor();
}

// *********************************************************
// ********************** loop() ***************************
// *********************************************************

void loop() {
  timeActual = millis();
  if (timeActual > (timeLastExecute + humidityTime()) || timeLastExecute == 0 || checkOpenRelay()) {
    getallServerOptions();
    diposit = mitjaDiposit();
    sendDiposit(diposit);
    timeLastExecute = millis();
    testMoistureLevel();
  }
}
