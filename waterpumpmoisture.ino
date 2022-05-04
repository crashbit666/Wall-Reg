#include <SPI.h>

const int pumpRelay[4] = { 2, 3, 4, 5 };
const int moistureSensor[4] = { A0, A1, A2, A3 };
const int ON = LOW;
const int OFF = HIGH;

int nivellHumitat[4]; // Inicialitza la variable sense valor per posteriorment recollira-la del servidor fb
int moistureLevelSensor[4];  

int freq;   //Inicialitza la variable sense valor. Ja agafa el valor del servidor firebase.
long unsigned retestHumidityTime = 60000 * freq; // Cada quanta estona es fa un test d'humitat

void initialize_waterPump() {
  //Inicialitza el relé i el sensor d'humitat
  for(int i = 0; i < 4; i++) {
    pinMode(pumpRelay[i], OUTPUT);
    digitalWrite(pumpRelay[i], OFF);  
    pinMode(moistureSensor[i], INPUT);
  }
  delay(500);
}

// Activa el relé i comença a regar.
void activateRelay(int i) {
  digitalWrite(pumpRelay[i], ON);
}

// Desactiva el relé i deixa de regar.
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
  checkOpenRelay();
}

// Això serveix per que si hi ha un relé obert (és a dir, està regant), no faci el següent test al cap de 5 segons i no el temps establert per servidor.
// De no fer-ho es podria donar el cas que un relé estigués fins a 60 minuts funcionant. Cosa molt perillosa.
void checkOpenRelay() {
  for(int i = 0; i < 4; i++) {
    if(digitalRead(pumpRelay[i]) == ON) {
      retestHumidityTime = 5000; // El motor no volem que es quedi engegat sense tornar a mirar la humitat més de 5 segons.
      break;                     // Aquest break, en teoria houria de deixar el valor de retestHumidityTime a 5 segons.
    } else {
      retestHumidityTime = 60000 * freq; // Torna a posar el temps de retest de la humitat a l'establert al servidor.
    }
  }
}

// Aquesta funció agafa els valors de les variables del servidor que posteriorment s'inicialitzaran al setup()
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
