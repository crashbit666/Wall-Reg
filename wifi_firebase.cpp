#include "Firebase_Arduino_WiFiNINA.h"
#include "arduino_secrets.h"
#include "Firebase_Arduino_WiFiNINA.h"

String path="/torretes";
FirebaseData fbdo;

//Paràmetres de la wifi
char ssidf[] = SECRET_SSIDF;
char passf[] = SECRET_PASSF;
char dbsf[] = SECRET_DBSF;

void initialize_wifi_firebase() {
  Serial.begin(115200);
  delay(100);
  
  //Connecta a la Wifi
  Serial.print("Connecting to Wi-Fi");
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED)
  {
    status = WiFi.begin(ssidf, passf);
    Serial.print(".");
    delay(100);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  //Dades d'autentificació
  Firebase.begin("wall-reg-default-rtdb.europe-west1.firebasedatabase.app", dbsf, ssidf, passf);
  Firebase.reconnectWiFi(true);

}

void showError() {
  Serial.println("FAILED");
  Serial.println("REASON: " + fbdo.errorReason());
  Serial.println("=================");
  Serial.println();
}

void sendData(int i, int valor) {
  int torreta;
  if (i<2) {
    torreta = 1;
  } else {
    torreta = 2;
  }
  Serial.println(path +  "/" + torreta +  "/sensor_humitat/" + i + valor);
  if (!Firebase.setInt(fbdo, path +  "/" + torreta +  "/Sensor_Humitat/" + i, valor)) {
    showError();
  }
}
