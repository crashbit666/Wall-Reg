/* TODO:
  * - Obtenir valors del servidor (Tema important)
  * - Afegir opció de saber el temps que farà. Per tal de poder predir si fa falta regar o en unes hores es posarà a ploure.
  * - Afeegir cèl·lula fotoelecèctrica per no regar si fa molt sol. (Mirar si la puc situar bé).
  * - Implementar actualitzacions del sketch via OTA (Sembla que no pot ser ja que la placa es <64kb)
  * - NO FUNCIONA el recheckserverOptions(); Sembla ser que ara hi ha 2 streams diferents i un dells amb 4 valors. Configurar això.
  * - Mirar bé lo d'ejecutar una funciió cada X temps
*/

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

unsigned long time1 = 0;
unsigned long timeEX = 0;
unsigned long timeFN = 0;

void setup() {
  //Inicialitza la Wifi i firebase
  initialize_wifi_firebase();

  //Inicialitza el relé i el sensor d'humitat
  initialize_waterPump();
}

void loop() {
  if (time1 > (timeEX + humidityTime()) || time1 ==0) {
    getallServerOptions();
    timeEX = millis();
    testMoistureLevel();
  }
  time1 = millis();
}
