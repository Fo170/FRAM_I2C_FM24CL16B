#include <Arduino.h>

#include <FRAM_I2C_FM24CL16B.h>
#include "FRAM_I2C_Exemples.h"

void setup() {
    Serial.begin(115200);
    Wire.begin();
    
    // Exécuter tous les exemples
    runAllExamples();
    
    // Ou un seul exemple spécifique
    // Exemple5_BufferCirculaire();
}

void loop() {

    //yield();
}