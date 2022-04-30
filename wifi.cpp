#include <WiFiNINA.h>
#include "arduino_secrets.h" 

// Paràmetres de la wifi
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;


void initialize_wifi() {
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
}