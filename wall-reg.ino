/* TODO:
  * - Afegir opció de saber el temps que farà. Per tal de poder predir si fa falta regar o en unes hores es posarà a ploure.
  * - Afeegir cèl·lula fotoelecèctrica per no regar si fa molt sol. (Mirar si la puc situar bé).
  * - Seria interessant saber si es pot detectar el nivell de bateria per gestionar un mode sleep mitjançant alguna targeta o semblant.
  * - Afegir protecció en cas de que el relé s'activi i la humitat no puji en un temps concret. (1 minuts aprox.).
  * - Gestió de l'energia per estalviar energia.
  * - Deshardcodificar les següents variables:
  * - Nombre de sensors (4), pero s'ha de hardcodificar si o si el PIN on està connectat el sensor.
*/

/* TEST:
 *  Pendent:
 *    - Test de les regles de firebase per saber si funcionen
 *    - Test de le lògica del programa, depurar-lo per saber si actua correctament. Especialment el temps entre checks si el relé està obert. Més que res
 *    s'hauria de mirar si ho fa correctament amb alguns relés oberts i altres tancats després de canvis en aquests.
 *  Fet:
 */

 /* FET:
  * - Els valor dels sensors s'envien correctament a la bbdd realtime de Firebase.
  * - Els valors de Firebase es carrguen correctament al codi com a variables.
  * - Implementació del sensor de distància del dipòsit.
*/

// IMPLEMETACIONS:
// - Firebase realtime database: https://github.com/mobizt/Firebase-Arduino-WiFiNINA


////// Variables servidor //////
// Nivells humitat: (humidityLevel)
// 500: Sec
// 425: Molt Baix
// 350: Bastant Baix
// 275: Normal
// 200: Bastant alt
// Nivells del sensor segons datasheet
// Més baix = més humit. 
// 100 -> Molt humit. 
// 700 -> completament sec.
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
#include <WiFiUdp.h>


// Variables per poder agafar la data actual
unsigned int localPort = 2390;      // Port local on escoltar els paquets UDP
IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp està als primers 48 bits del missatge
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer on es mantenen els paquets pendents d'enviar/rebre.

String hour, minutes, seconds;

// Una instància UDP per deixar-nos enviar i rebre paquets UDP
WiFiUDP Udp;

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
byte freq;

// Inicialitza paràmetres del relé i els sensors. PINS dels relés i del sensors d'humitat.
const int pumpRelay[4] = { 2, 3, 4, 5 };
const int moistureSensor[4] = { A0, A1, A2, A3 };
const int ON = LOW;
const int OFF = HIGH;
bool rele[4] = { false, false, false, false };

// Inicialitza la variable sense valor per posteriorment recollira-la del servidor fb
int nivellHumitat[4];
int moistureLevelSensor[4];  


// ********************************************************
// *            Inici de les funcions                     *
// ********************************************************

// Funció per inicialitzar relé i sensor d'humitat.
void initialize_waterPump() {
  //Inicialitza el relé i el sensor d'humitat
  for(byte i = 0; i < 4; i++) {
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
He triat 250, ja que no disposo de cap dipòsit superior i per tant la distància seria erronea.
Pendent de mesurar el dipòsit i canviar el 400mm per valor corresponent.
*/
int nivellDiposit() {

  unsigned char data[4]={};
  int distance;

  do {
    for (byte i=0; i<4; i++) {
      data[i] = mySerial.read();
    }
  } while (mySerial.read() == 0xff);

  mySerial.flush();

  if (data[0] == 0xff) {
    int sum;
    sum = (data[0]+data[1]+data[2])&0x00FF;
    if (sum == data[3]) {
      distance = (data[1]<<8)+data[2];
      if ((distance > 30) && (distance < 250)) {
        //Serial.print("distance=");
        //Serial.print(distance);
        //Serial.println("mm");
        delay(100);
        return distance;
      } else if (distance >= 400) {
        //Serial.println("Empty tank");
        delay(100);
        return 0;
      } else {
        //Serial.println("Full tank");
        delay(100);
        return 1;
      }
    } else {
      //Serial.println("Checksum error");
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
// - distancia: La distància mesurada.
// - -1: Error al mesurar la distància.
// - -2: No s'ha pogut mesurar la distància, ja que el sensor no ha facilitat cap dada.

int mitjaDiposit() {
  int suma = 0;
  byte error = 0;
  byte buit = 0;
  int tmp = 0;
  byte count = 0;
  byte ple = 0;

  for (int x = 0; x < 20; x++) {
    tmp = nivellDiposit();
    //Serial.println("tmp: " + String(tmp));
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
  //Serial.println("error: " + String(error));
  //Serial.println("buit: " + String(buit));
  //Serial.println("ple: " + String(ple));
  //Serial.println("count: " + String(count));
  //Serial.println("suma: " + String(suma));
  //Serial.println("mitja: " + String(suma/count));
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
  setWaterPumpStatus(i, true);
  registerLastWattering(i);
  rele(i) = true;
  delay(5000);
}

// Funció que desactiva el relé i deixa de regar.
void deactivateRelay(int i) {
  // Aquí potser es tindria que afegir un comprobació per saber si ja està parat
  digitalWrite(pumpRelay[i], OFF);
  setWaterPumpStatus(i, false);
  rele(i) = false;
}

// Comprova si els nivells d'humitat són els adequats, de no ser així activa/desactiva el relé.
void testMoistureLevel() {
  do {
    for (byte i = 0; i < 4; i++) {
      moistureLevelSensor[i] = analogRead(moistureSensor[i]); // Lectura del sensor de humitat
      sendData(i,moistureLevelSensor[i]); // Envia les dades a la bbdd firebase.
      if(moistureLevelSensor[i] > nivellHumitat[i]) {
        if ((diposit != -1) && (diposit != 0) && (diposit != 1)) { // Si el dipòsit està buit, la medició ha donat error o marca com a ple no activis el relé.
          activateRelay(i);
        }
      }
    }
  } while (checkOpenRelay());
}

// Aquest funció serveix per que si hi ha un relé obert (és a dir, està regant), no faci el següent test al cap de 5 segons i no el temps establert per servidor.
// De no fer-ho es podria donar el cas que un relé estigués fins a 60 minuts funcionant.
// Arreglat el return. Només retorna el true dins del bucle. El false sempre fora del bucle.
bool checkOpenRelay() {
  //int status = 0;
  for(byte i = 0; i < 4; i++) {
    ////Serial.print("RELAY ");
    ////Serial.print(i);
    if(reles[i] == true) {
      //Serial.println("INTERRUPCIÓ DEL BUCLE RELÉ OBERT");
      return true;
    }
    ////Serial.println(" ..... OK");
  }
  ////Serial.println("NO HI HA RELÉS OBERTS");
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
  ////Serial.print("freq = ");
  ////Serial.println(freq);
  return 60000 * freq; 
}

// La següent funció inicialitza els paràmetres del servidor firebase.
void initialize_wifi_firebase() {
  Serial.begin(57600);
  //delay(100);
  
  //Connecta a la Wifi
  //Serial.print("Connectant a la Wi-Fi");
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED)
  {
    status = WiFi.begin(ssidf, passf);
    //Serial.print(".");
    delay(100);
  }
  //Serial.println();
  //Serial.print("Connectat amb IP: ");
  //Serial.println(WiFi.localIP());
  //Serial.println();
  
  //Dades d'autentificació
  Firebase.begin(fbhost, dbsf, ssidf, passf);
  Firebase.reconnectWiFi(true);
}

// Mostra errors relacionats amb la inicialització i enviament de dades
void showError() {
  //Serial.println("FAILED");
  //Serial.println("REASON: " + fbdo.errorReason());
  //Serial.println("=================");
  //Serial.println();

  //Aquí desactivo els relés per evitar que problemes de conexió puguin deixar el reg permanentment engegat.
  for (byte i=0; i<4; i++) {
    deactivateRelay(i);
  }
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
void setWaterPumpStatus(byte i, bool status) {
  if (!Firebase.setBool(fbdo, path + "/bombes/" +i, status)) {
    showError();
  }
}

void sendDiposit(int i) {
  if (!Firebase.setInt(fbdo, path + "/Diposit", diposit)) {
    showError();
  }
}



// Aquesta funció envia les dades dels sensor d'humitat al servidor firebase. La primera part detecta la torreta per després poder treballar amb app mobil.
void sendData(byte i, int valor) {
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
byte getdataFreq() {
  if (Firebase.getInt(fbdo, path + "/frecuencia")) {
    byte fq = fbdo.intData();
    //fbdo.clear();
    return fq;
  } else {
    showError();
  }
}

// Funció que retorna els valors configurats de nivell d'humitat. 
// La funció retorna un punter, ja que interessa recuperar les dades com un array.
int * getdataNivellHumitat() {
  static int hl[4];
  for (byte i = 0; i < 4; i++) {
    if (Firebase.getInt(fbdo, path + "/Config_Humidity/" + i)) {
      hl[i] = fbdo.intData();
    } else {
      showError();
    }
  }
  //fbdo.clear();
  return hl;
}

//Aquesta funció es un contador. L'utilitzo per depurar el programa desde firebase. Aquí veig un valor que canvia en cada ietraccio del if principal.
void lastCheck() {
  getDate();
  if (!Firebase.setString(fbdo, path + "/lastCheck", hour + ":" + minutes + ":" + seconds)) {
    showError();
  }  
}

// Funció que retorna l'hora actual del servidor NTP
void getDate() {
  sendNTPpacket(timeServer);
  delay(1000);
  if (Udp.parsePacket()) {
    Udp.read(packetBuffer, NTP_PACKET_SIZE);
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;
    hour = (epoch  % 86400L) / 3600;
    minutes = (epoch  % 3600) / 60;
    seconds = epoch % 60;
  }
}

  // send an NTP request to the time server at the given address
  unsigned long sendNTPpacket(IPAddress& address) {
  ////Serial.println("1");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  ////Serial.println("2");
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  ////Serial.println("3");

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  ////Serial.println("4");
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  ////Serial.println("5");
  Udp.endPacket();
  ////Serial.println("6");
}

void registerLastWattering(int i) {
  getDate();
  if (!Firebase.setString(fbdo, path + "/lastWattering", hour + ":" + minutes + ":" + seconds + " | torreta-> " + i)) {
    showError();
    //
  }
}

/*
// Aquesta funció informa si estem en mode de depuració .
byte getDebugMode() {
  if (Firebase.getInt(fbdo, path + "/debugMode")) {
    byte dm = fbdo.intData();
    fbdo.clear();
    return dm;
  } else {
    showError();
  }
}
void log(string sMessage){
  if () {
    Firebase.pushString(fbdo, path + "/bombes/" +i, status)
  }
}
*/

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

   //Inicialitza el port UDP per NTP
  Udp.begin(localPort);
}

// *********************************************************
// ********************** loop() ***************************
// *********************************************************

void loop() {
  timeActual = millis();
  if (timeActual > (timeLastExecute + humidityTime()) || timeLastExecute == 0 || checkOpenRelay()) {
    lastCheck();
    getallServerOptions();
    diposit = mitjaDiposit();
    sendDiposit(diposit);
    timeLastExecute = millis();
    testMoistureLevel();
  }
}
