/* TODO:
  * - Obtenir valors del servidor (Tema important)
  * - Afegir opció de saber el temps que farà. Per tal de poder predir si fa falta regar o en unes hores es posarà a ploure.
  * - Afeegir cèl·lula fotoelecèctrica per no regar si fa molt sol. (Mirar si la puc situar bé).
  * - Implementar actualitzacions del sketch via OTA (Sembla que no pot ser ja que la placa es <64kb)
*/

#include "waterpumpmoisture.h"
#include "wifi_firebase.h"

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
  //Inicialitza la Wifi i firebase
  initialize_wifi_firebase();

  //Inicialitza el relé i el sensor d'humitat
  initialize_waterPump();
}

void loop() {
  testMoistureLevel();
}