/*
    FRAM_I2C_FM24CL16B_Exemples.h v2.0
    ===================================
    
    Exemples d'utilisation de la librairie FRAM_I2C_FM24CL16B
    Gestion multi-puces FM24CL16B avec protection mémoire
    
    Compatible avec ESP8266, ESP32, Arduino Uno/Nano/Mega
*/

#ifndef FRAM_I2C_FM24CL16B_EXEMPLES_H
#define FRAM_I2C_FM24CL16B_EXEMPLES_H

#include "FRAM_I2C_FM24CL16B.h"

// Wrappers pour compatibilité (paramètre Serial par défaut)
#define printError(e) printError(e, Serial)
#define printFRAMInfo(f) printFRAMInfo(f, Serial)
/*
inline void printError(fram_error_t err) {
    printError(err, Serial);
}

inline void printFRAMInfo(FRAM_FM24CL16B &fram) {
    printFRAMInfo(fram, Serial);
}
*/
// ============================================
// CONFIGURATION DES ADRESSES MÉMOIRE
// ============================================

// Structure de données exemple
struct vvv {
    byte a;
    int b;
    long c;
};

// Adresses pour les exemples (dans les 2KB de la FM24CL16B)
#define ADR_COMPTEUR        0x0000  // uint32_t - Compteur de boot
#define ADR_STRUCT_VVV      0x0010  // struct vvv
#define ADR_INT_ARRAY       0x0020  // Tableau de 10 int
#define ADR_FLOAT_PI        0x0050  // float
#define ADR_FLOAT_2PI       0x0060  // float
#define ADR_DOUBLE_E        0x0070  // double
#define ADR_STRING          0x0080  // char[32]
#define ADR_SENSOR_DATA     0x00C0  // Données capteur
#define ADR_CIRCULAR_LOG    0x0100  // Buffer circulaire
#define ADR_MULTI_CHIP      0x0400  // Test multi-puces

// ============================================
// EXEMPLE 1: Initialisation et test basique
// ============================================

void Exemple1_BasicInit() {
    Serial.println("\n=== EXEMPLE 1: Initialisation Basique ===");
    
    FRAM_FM24CL16B fram;
    if(!fram.begin(400000)) {
        Serial.println("ERREUR: FRAM non détectée!");
        return;
    }
    
    Serial.println("FRAM initialisée avec succès!");
    printFRAMInfo(fram);
    
    fram_error_t err;
    
    // TEST PAGE 0 uniquement (adresse I2C 0x50)
    Serial.println("Test ecriture @0x0000 (page 0)...");
    err = fram.writeByte(0x0000, 0x42);
    Serial.print("Write: "); printError(err);
    
    Serial.println("Test lecture @0x0000 (page 0)...");
    uint8_t val = fram.readByte(0x0000);
    Serial.print("Read: 0x"); Serial.println(val, HEX);
    
    // NE PAS tester 0x0100 pour l'instant
    
    Serial.println("FIN Exemple1");
}

void Exemple1_BasicInit_() {
    Serial.println("\n=== EXEMPLE 1: Initialisation Basique ===");
    
    // Création instance avec adresse par défaut (0x50)
    FRAM_FM24CL16B fram;
    
    // Initialisation I2C à 400kHz
    if(!fram.begin(400000)) {
        Serial.println("ERREUR: FRAM non détectée!");
        return;
    }
    
    Serial.println("FRAM initialisée avec succès!");
    printFRAMInfo(fram);
    
    // Test écriture/lecture basique
    fram_error_t err;
    
    // Écriture octet
    err = fram.writeByte(0x0100, 0x42);
    Serial.print("Write byte: ");
    printError(err);
    
    uint8_t val = fram.readByte(0x0100);
    Serial.print("Read byte: 0x");
    Serial.println(val, HEX);
    
    // Test protection mémoire
    Serial.println("\nTest protection:");
    // Alternative : ne pas tester avec 3000 octets
    uint8_t buffer[16];             // tient facilement dans la stack
    err = fram.write(0x07F0, buffer, 32);  // 0x7F0+32 = 0x810 > 0x7FF
    printError(err);                // Doit afficher ADDRESS OVERFLOW
    /*uint8_t buffer[3000];
    err = fram.write(0x0000, buffer, 3000);  // Trop grand!*/
    printError(err);  // Doit afficher FRAM_ERROR_ADDR_OVERFLOW
}

// ============================================
// EXEMPLE 2: Types de données variés
// ============================================

void Exemple2_TypesDeDonnees() {
    Serial.println("\n=== EXEMPLE 2: Types de Données ===");
    
    FRAM_FM24CL16B fram(FRAM_ADDR_000);
    if(!fram.begin()) {
        Serial.println("FRAM non détectée!");
        return;
    }
    
    fram_error_t err;
    
    // 1. uint8_t
    Serial.println("\n-- uint8_t --");
    err = fram.writeByte(ADR_COMPTEUR, 0x99);
    uint8_t b = fram.readByte(ADR_COMPTEUR);
    Serial.print("Byte: 0x");
    Serial.println(b, HEX);
    
    // 2. int16_t
    Serial.println("\n-- int16_t --");
    int16_t si = -12345;
    err = fram.writeInt(ADR_INT_ARRAY, si);
    int16_t si_read;
    fram.readInt(ADR_INT_ARRAY, &si_read);
    Serial.print("Int: ");
    Serial.println(si_read);
    
    // 3. int32_t (long)
    Serial.println("\n-- int32_t --");
    int32_t li = 123456789L;
    err = fram.writeLong(ADR_INT_ARRAY + 4, li);
    int32_t li_read;
    fram.readLong(ADR_INT_ARRAY + 4, &li_read);
    Serial.print("Long: ");
    Serial.println(li_read);
    
    // 4. float
    Serial.println("\n-- float --");
    float f = 3.14159265;
    err = fram.writeFloat(ADR_FLOAT_PI, f);
    float f_read;
    fram.readFloat(ADR_FLOAT_PI, &f_read);
    Serial.print("Float: ");
    Serial.println(f_read, 8);
    
    // 5. double
    Serial.println("\n-- double --");
    double d = 2.718281828459045;
    err = fram.writeDouble(ADR_DOUBLE_E, d);
    double d_read;
    fram.readDouble(ADR_DOUBLE_E, &d_read);
    Serial.print("Double: ");
    Serial.println(d_read, 15);
    
    // 6. Tableau d'entiers
    Serial.println("\n-- Tableau int[6] --");
    int val[6] = {0x03, 0x14, 0x15, 0x92, 0x65, 0x36};
    err = fram.write(ADR_INT_ARRAY, val, sizeof(val));
    
    int val_read[6];
    err = fram.read(ADR_INT_ARRAY, val_read, sizeof(val_read));
    Serial.print("Array: ");
    for(int i=0; i<6; i++) {
        Serial.print("0x");
        Serial.print(val_read[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    // 7. Chaîne de caractères
    Serial.println("\n-- String --");
    char hello[] = "Hello, FRAM v2.0!";
    err = fram.write(ADR_STRING, hello, strlen(hello)+1);
    
    char hhh[32];
    err = fram.read(ADR_STRING, hhh, sizeof(hhh));
    Serial.print("String: ");
    Serial.println(hhh);
    
    // 8. Structure
    Serial.println("\n-- Structure --");
    struct vvv abc;
    abc.a = 10;
    abc.b = 100;
    abc.c = 1000;
    err = fram.write(ADR_STRUCT_VVV, &abc, sizeof(abc));
    
    struct vvv cab;
    err = fram.read(ADR_STRUCT_VVV, &cab, sizeof(cab));
    Serial.print("Struct: a=");
    Serial.print(cab.a);
    Serial.print(", b=");
    Serial.print(cab.b);
    Serial.print(", c=");
    Serial.println(cab.c);
}

// ============================================
// EXEMPLE 3: Gestion d'erreurs avancée
// ============================================

void Exemple3_GestionErreurs() {
    Serial.println("\n=== EXEMPLE 3: Gestion des Erreurs ===");
    
    FRAM_FM24CL16B fram;
    fram.begin();
    
    fram_error_t err;
    uint8_t buffer[100];
    
    // Test 1: Adresse valide
    Serial.println("\n1. Écriture valide @0x0100");
    err = fram.write(0x0100, buffer, 50);
    printError(err);
    
    // Test 2: Dépassement capacité
    Serial.println("\n2. Dépassement @0x07F0 + 32 octets");
    err = fram.write(0x07F0, buffer, 32);  // 0x7F0 + 32 = 0x810 > 0x7FF
    printError(err);
    
    // Test 3: Adresse hors limites
    Serial.println("\n3. Adresse hors limites @0x0800");
    err = fram.write(0x0800, buffer, 10);  // 0x800 = 2048, max = 2047
    printError(err);
    
    // Test 4: Pointeur NULL
    Serial.println("\n4. Pointeur NULL");
    err = fram.write(0x0100, nullptr, 10);
    printError(err);
    
    // Test 5: Longueur nulle
    Serial.println("\n5. Longueur nulle");
    err = fram.write(0x0100, buffer, 0);
    printError(err);
    
    // Test 6: Écriture à la limite exacte
    Serial.println("\n6. Écriture limite exacte @0x07F0 + 16");
    err = fram.write(0x07F0, buffer, 16);  // 0x7F0 + 16 = 0x800 = OK
    printError(err);
    
    // Test 7: Lecture hors limites
    Serial.println("\n7. Lecture hors limites");
    err = fram.read(0x07FF, buffer, 10);  // 0x7FF + 10 = 0x809 > 0x7FF
    printError(err);
}

// ============================================
// EXEMPLE 4: Multi-puces (jusqu'à 8 FRAM)
// ============================================

void Exemple4_MultiPuces() {
    Serial.println("\n=== EXEMPLE 4: Gestion Multi-Puces ===");
    
    // Configuration pour 2 puces (nécessite hardware)
    // Puce 1: A0=A1=A2=GND -> 0x50
    // Puce 2: A0=VCC, A1=A2=GND -> 0x51
    
    FRAM_FM24CL16B fram1(FRAM_ADDR_000);  // 0x50
    FRAM_FM24CL16B fram2(FRAM_ADDR_001);  // 0x51
    
    Serial.println("Test Puce 1 (0x50)...");
    if(fram1.begin()) {
        Serial.println("  Puce 1 détectée!");
        fram1.writeByte(0x0000, 0x11);
        Serial.print("  Lu: 0x");
        Serial.println(fram1.readByte(0x0000), HEX);
    } else {
        Serial.println("  Puce 1 non détectée");
    }
    
    Serial.println("Test Puce 2 (0x51)...");
    if(fram2.begin()) {
        Serial.println("  Puce 2 détectée!");
        fram2.writeByte(0x0000, 0x22);
        Serial.print("  Lu: 0x");
        Serial.println(fram2.readByte(0x0000), HEX);
    } else {
        Serial.println("  Puce 2 non détectée (normal si non connectée)");
    }
    
    // Utilisation du Manager pour adressage global
    Serial.println("\nUtilisation FRAM_Manager...");
    FRAM_Manager manager;
    
    if(manager.addDevice(FRAM_ADDR_000)) {
        Serial.println("  Ajout puce 0: OK");
    }
    if(manager.addDevice(FRAM_ADDR_001)) {
        Serial.println("  Ajout puce 1: OK");
    }
    
    Serial.print("Total mémoire: ");
    Serial.print(manager.getTotalSize());
    Serial.println(" octets");
    
    // Écriture/lecture globale
    if(manager.getDeviceCount() >= 2) {
        char test[] = "Cross-chip test!";
        manager.writeGlobal(0x07F0, test, strlen(test)+1);  // Fin puce 1
        
        char readbuf[32];
        manager.readGlobal(0x07F0, readbuf, strlen(test)+1);
        Serial.print("Lu @0x07F0: ");
        Serial.println(readbuf);
        
        // Test écriture traversant les puces
        uint8_t data[40];
        for(int i=0; i<40; i++) data[i] = i;
        manager.writeGlobal(0x07F0, data, 40);  // 16 octets puce1 + 24 puce2
        
        uint8_t readdata[40];
        manager.readGlobal(0x07F0, readdata, 40);
        bool ok = true;
        for(int i=0; i<40; i++) if(readdata[i] != i) ok = false;
        Serial.print("Cross-chip write: ");
        Serial.println(ok ? "OK" : "ERREUR");
    }
}

// ============================================
// EXEMPLE 5: Buffer circulaire pour logging
// ============================================

// Structure pour données de log
struct LogData {
    uint32_t timestamp;
    float value1;
    float value2;
    uint8_t status;
};

#define LOG_SIZE 50  // 50 entrées max
#define LOG_ENTRY_SIZE sizeof(LogData)

void Exemple5_BufferCirculaire() {
    Serial.println("\n=== EXEMPLE 5: Buffer Circulaire ===");
    
    FRAM_FM24CL16B fram;
    fram.begin();
    
    // Adresses de gestion du buffer
    const uint16_t ADR_LOG_HEAD = ADR_CIRCULAR_LOG;           // uint16_t
    const uint16_t ADR_LOG_COUNT = ADR_CIRCULAR_LOG + 2;      // uint16_t
    const uint16_t ADR_LOG_DATA = ADR_CIRCULAR_LOG + 4;       // Début données
    
    // Lecture état actuel
    uint16_t head = 0, count = 0;
    fram.read(ADR_LOG_HEAD, &head, sizeof(head));
    fram.read(ADR_LOG_COUNT, &count, sizeof(count));
    
    Serial.print("Head actuel: ");
    Serial.println(head);
    Serial.print("Count actuel: ");
    Serial.println(count);
    
    // Ajout nouvelle entrée
    LogData newLog;
    newLog.timestamp = millis();
    newLog.value1 = random(1000) / 10.0;
    newLog.value2 = random(500) / 10.0;
    newLog.status = random(256);
    
    uint16_t writeAddr = ADR_LOG_DATA + (head * LOG_ENTRY_SIZE);
    fram_error_t err = fram.write(writeAddr, &newLog, LOG_ENTRY_SIZE);
    
    if(err == FRAM_OK) {
        head = (head + 1) % LOG_SIZE;
        if(count < LOG_SIZE) count++;
        
        fram.write(ADR_LOG_HEAD, &head, sizeof(head));
        fram.write(ADR_LOG_COUNT, &count, sizeof(count));
        
        Serial.println("Log ajouté avec succès!");
    } else {
        Serial.print("Erreur écriture: ");
        printError(err);
    }
    
    // Affichage des 5 dernières entrées
    Serial.println("\n5 dernières entrées:");
    uint16_t start = (head + LOG_SIZE - min((int)count, 5)) % LOG_SIZE;
    
    for(int i=0; i<5 && i<count; i++) {
        uint16_t idx = (start + i) % LOG_SIZE;
        uint16_t addr = ADR_LOG_DATA + (idx * LOG_ENTRY_SIZE);
        
        LogData log;
        fram.read(addr, &log, sizeof(log));
        
        Serial.print("  [");
        Serial.print(idx);
        Serial.print("] T=");
        Serial.print(log.timestamp);
        Serial.print("ms, V1=");
        Serial.print(log.value1);
        Serial.print(", V2=");
        Serial.print(log.value2);
        Serial.print(", S=0x");
        Serial.println(log.status, HEX);
    }
}

// ============================================
// EXEMPLE 6: Dump et manipulation mémoire
// ============================================

void Exemple6_DumpEtManipulation() {
    Serial.println("\n=== EXEMPLE 6: Dump et Manipulation ===");
    
    FRAM_FM24CL16B fram;
    fram.begin();
    
    // Remplissage avec pattern
    Serial.println("Remplissage 0x0200-0x02FF avec pattern...");
    for(int i=0; i<256; i++) {
        fram.writeByte(0x0200 + i, i);
    }
    
    // Dump hexadécimal
    Serial.println("\nDump 0x0200-0x023F (64 octets):");
    fram.dump(0x0200, 64, Serial);
    
    // Effacement partiel
    Serial.println("\nEffacement 0x0210-0x021F...");
    fram.clear(0x0210, 16);
    fram.dump(0x0200, 64, Serial);
    
    // Remplissage avec valeur
    Serial.println("\nRemplissage 0x0220-0x022F avec 0xAA...");
    fram.fill(0x0220, 16, 0xAA);
    fram.dump(0x0200, 64, Serial);
    
    // Dump complet (décommenter si nécessaire - long!)
    // Serial.println("\nDump complet mémoire:");
    // fram.dumpAll(Serial);
}

// ============================================
// EXEMPLE 7: Compteur persistant (anti-reset)
// ============================================

void Exemple7_CompteurPersistant() {
    Serial.println("\n=== EXEMPLE 7: Compteur Persistant ===");
    
    FRAM_FM24CL16B fram;
    fram.begin();
    
    uint32_t bootCount = 0;
    fram_error_t err = fram.read(ADR_COMPTEUR, &bootCount, sizeof(bootCount));
    
    if(err != FRAM_OK) {
        Serial.println("Première initialisation!");
        bootCount = 0;
    }
    
    Serial.print("Nombre de boots précédents: ");
    Serial.println(bootCount);
    
    bootCount++;
    err = fram.write(ADR_COMPTEUR, &bootCount, sizeof(bootCount));
    
    if(err == FRAM_OK) {
        Serial.print("Nouveau compteur sauvegardé: ");
        Serial.println(bootCount);
    } else {
        Serial.print("Erreur sauvegarde: ");
        printError(err);
    }
}

// ============================================
// EXEMPLE 8: Sauvegarde configuration système
// ============================================

struct SystemConfig {
    uint16_t magic;           // 0x1234 pour valider
    uint8_t version;
    uint16_t logInterval;     // Minutes
    float calibration[4];
    char deviceName[16];
    uint32_t totalRuntime;    // Secondes
    uint16_t checksum;
};

void Exemple8_ConfigurationSysteme() {
    Serial.println("\n=== EXEMPLE 8: Configuration Système ===");
    
    FRAM_FM24CL16B fram;
    fram.begin();
    
    const uint16_t ADR_CONFIG = 0x0300;
    fram_error_t err;
    
    // Écriture configuration
    SystemConfig config;
    config.magic = 0x1234;
    config.version = 2;
    config.logInterval = 10;  // 10 minutes
    config.calibration[0] = 1.0;
    config.calibration[1] = 0.0;
    config.calibration[2] = 1.0;
    config.calibration[3] = 0.0;
    strcpy(config.deviceName, "ESP8266_FRAM");
    config.totalRuntime = 123456;
    config.checksum = 0;  // À calculer en production
    
    err = fram.write(ADR_CONFIG, &config, sizeof(config));
    Serial.print("Sauvegarde config: ");
    printError(err);
    
    // Lecture et vérification
    SystemConfig configRead;
    err = fram.read(ADR_CONFIG, &configRead, sizeof(configRead));
    
    if(err == FRAM_OK && configRead.magic == 0x1234) {
        Serial.println("Configuration valide lue:");
        Serial.print("  Version: ");
        Serial.println(configRead.version);
        Serial.print("  Device: ");
        Serial.println(configRead.deviceName);
        Serial.print("  Log interval: ");
        Serial.print(configRead.logInterval);
        Serial.println(" min");
        Serial.print("  Runtime: ");
        Serial.print(configRead.totalRuntime);
        Serial.println(" sec");
    } else {
        Serial.println("Configuration invalide ou erreur lecture!");
    }
}

// ============================================
// EXEMPLE 9: Test de performance
// ============================================

void Exemple9_Performance() {
    Serial.println("\n=== EXEMPLE 9: Test Performance ===");
    
    FRAM_FM24CL16B fram;
    fram.begin();
    
    uint8_t buffer[256];
    for(int i=0; i<256; i++) buffer[i] = i;
    
    // Test écriture 256 octets
    unsigned long start = micros();
    fram_error_t err = fram.write(0x0400, buffer, 256);
    unsigned long writeTime = micros() - start;
    
    Serial.print("Write 256 bytes: ");
    Serial.print(writeTime);
    Serial.println(" us");
    printError(err);
    
    // Test lecture 256 octets
    uint8_t readbuf[256];
    start = micros();
    err = fram.read(0x0400, readbuf, 256);
    unsigned long readTime = micros() - start;
    
    Serial.print("Read 256 bytes: ");
    Serial.print(readTime);
    Serial.println(" us");
    printError(err);
    
    // Vérification intégrité
    bool ok = true;
    for(int i=0; i<256; i++) {
        if(readbuf[i] != buffer[i]) ok = false;
    }
    Serial.print("Data integrity: ");
    Serial.println(ok ? "OK" : "FAILED");
    
    // Test écriture octet par octet (comparaison)
    start = micros();
    for(int i=0; i<256; i++) {
        fram.writeByte(0x0500 + i, buffer[i]);
    }
    unsigned long byteWriteTime = micros() - start;
    
    Serial.print("Write 256x1 byte: ");
    Serial.print(byteWriteTime);
    Serial.println(" us");
    Serial.print("Gain bloc vs byte: ");
    Serial.print((byteWriteTime * 100) / writeTime);
    Serial.println("%");
}

// ============================================
// EXEMPLE 10: Mode basse consommation (simulation)
// ============================================

void Exemple10_LowPowerMode() {
    Serial.println("\n=== EXEMPLE 10: Mode Économie ===");
    
    FRAM_FM24CL16B fram;
    fram.begin(100000);  // 100kHz pour économie d'énergie
    
    // Opérations rapides puis retour sommeil
    uint16_t sensorValue = analogRead(A0);
    fram.write(0x0600, &sensorValue, sizeof(sensorValue));
    
    Serial.println("Donnée sauvegardée à 100kHz (économie d'énergie)");
    Serial.println("Le FRAM conserve les données sans alimentation!");
}

// ============================================
// FONCTION PRINCIPALE DE DÉMONSTRATION
// ============================================

void runAllExamples() {
    Serial.println("\n");
    Serial.println("╔══════════════════════════════════════════════════╗");
    Serial.println("║    FRAM_I2C_FM24CL16B Exemples Complets v2.0     ║");
    Serial.println("╚══════════════════════════════════════════════════╝");
    
    Exemple1_BasicInit();
    delay(100);
    
    Exemple2_TypesDeDonnees();
    delay(100);
    
    Exemple3_GestionErreurs();
    delay(100);
    
    Exemple4_MultiPuces();
    delay(100);
    
    Exemple5_BufferCirculaire();
    delay(100);
    
    Exemple6_DumpEtManipulation();
    delay(100);
    
    Exemple7_CompteurPersistant();
    delay(100);
    
    Exemple8_ConfigurationSysteme();
    delay(100);
    
    Exemple9_Performance();
    delay(100);
    
    Exemple10_LowPowerMode();
    
    Serial.println("\n");
    Serial.println("════════════════════════════════════════════════════");
    Serial.println("    Tous les exemples ont été exécutés!");
    Serial.println("════════════════════════════════════════════════════");
}

#endif // FRAM_I2C_FM24CL16B_EXEMPLES_H