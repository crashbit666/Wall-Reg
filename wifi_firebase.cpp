#include "Firebase_Arduino_WiFiNINA.h"
#include "arduino_secrets.h"

String path="/torretes";
FirebaseData fbdo;

//Paràmetres de la wifi
char ssidf[] = SECRET_SSIDF;
char passf[] = SECRET_PASSF;
char dbsf[] = SECRET_DBSF;
char fbhost[] = SECRET_FBHOST;

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
 *    weather (bool)
 *      
 *   
*/

// Aquesta funció envia les dades dels sensor d'humitat al servidor firebase.
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
  return hl;
}
