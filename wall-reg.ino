/* TODO:
  * - Obtenir valors del servidor
  * - Afegir opció de saber el temps que farà.
  * - Afeegir cèl·lula fotoelecèctrica per no regar si fa molt sol.
  * - Implementar actualitzacions del sketch via OTA (Sembla que no pot ser, <64kb)
*/

#include "wifi.h"
#include "waterpumpmoisture.h"

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


//Més baix = més humit. 
//275 -> Molt humit. 
//600 -> completament sec.

void setup() {
  //Inicialitza la consola
  Serial.begin(9600);

  //Inicialitza la Wifi
  initialize_wifi();

  //Inicialitza el relé i el sensor d'humitat
  initialize_waterPump();
}


void loop() {
  for(int i = 0; i < 4; i++) {
    testMoistureLevel(i);
  }
}
