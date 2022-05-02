#include <SPI.h>
#include "waterpumpmoisture.h"
#include "wifi_firebase.h"

const int pumpRelay[4] = { 2, 3, 4, 5 };
const int moistureSensor[4] = { A0, A1, A2, A3 };
const int ON = LOW;
const int OFF = HIGH;

int humidityLevel = 450;
int nivellHumitat[4] = { 450, 450, 450, 450 }; // TODO: Aquí tindria que agafar els nivells d'umitat marcats al servidor.
int moistureThreshold[4] = { nivellHumitat[0], nivellHumitat[1], nivellHumitat[2], nivellHumitat[3] };  // Ajustar a les necesitats de cada planta
int moistureLevel[4];  


int freq = 1;                           // TODO: Aquí tindria que agafar el valor del servidor
long retestHumidityTime = 60000 * freq; // Cada quanta estona es fa un test d'humitat

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
    moistureLevel[i] = analogRead(moistureSensor[i]);
    sendData(i,moistureLevel[i]);
    if(moistureLevel[i] > moistureThreshold[i]) {
      activateRelay(i);
    } else {
      deactivateRelay(i);
    }
  }
  checkOpenRelay();
  delay(retestHumidityTime);
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
