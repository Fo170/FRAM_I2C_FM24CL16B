/*
    Test_FRAM_Complet.ino
    =====================
    Programme de test complet pour FRAM FM24CL16B (I2C)
    Compatible ESP8266 / ESP32 / Arduino

    Branchement :
      FRAM VCC  -> 3.3V
      FRAM GND  -> GND
      FRAM SDA  -> GPIO4 (ESP8266 D2) ou SDA
      FRAM SCL  -> GPIO5 (ESP8266 D1) ou SCL
      FRAM WP   -> 3.3V (optionnel, desactive l'ecriture si GND)
      FRAM A0-A2 -> GND (adresse 0x50)

# Voici un programme de test complet et autonome pour valider votre FRAM FM24CL16B.

## 📋 Ce que le test vérifie

| #  | Test                     | Description                                                    |
| -- | ------------------------ | -------------------------------------------------------------- |
| 1  | **Initialisation I2C**   | Détection de la puce sur le bus                                |
| 2  | **Octet basique**        | Écriture/lecture de patterns (0x00, 0xFF, 0x55, 0xAA...)       |
| 3  | **Frontière de page**    | Accès aux adresses critiques (0x00FF→0x0100, 0x01FF→0x0200...) |
| 4  | **Bloc séquentiel**      | Écriture/lecture de 256 octets d'un coup                       |
| 5  | **Types de données**     | `int16`, `int32`, `float`, `double`, `struct`, `string`        |
| 6  | **Protection d'adresse** | Vérifie que l'overflow est bien détecté                        |
| 7  | **Clear / Fill**         | Remplissage à 0x00 et 0xAA                                     |
| 8  | **Persistance**          | Compteur de boot avec magic value (survît au reset)            |
| 9  | **Walk mémoire**         | Test rapide sur toute la mémoire (tous les 64 octets)          |
| 10 | **Stress**               | 10 cycles écriture/lecture intensifs                           |
*/

#include <Wire.h>
#include "FRAM_I2C_FM24CL16B.h"

// ============================================
// CONFIGURATION
// ============================================
#define SERIAL_BAUD     115200
#define I2C_SPEED       400000      // 400 kHz

// ============================================
// STATISTIQUES DE TEST
// ============================================
struct TestStats {
    uint16_t total;
    uint16_t passed;
    uint16_t failed;
};

static TestStats stats = {0, 0, 0};

// ============================================
// MACROS DE TEST
// ============================================
#define TEST_START(name)  do { Serial.print("\n[TEST] "); Serial.print(name); Serial.print(" ... "); stats.total++; } while(0)
#define TEST_PASS()       do { Serial.println("PASS"); stats.passed++; } while(0)
#define TEST_FAIL(msg)    do { Serial.print("FAIL : "); Serial.println(msg); stats.failed++; } while(0)
#define ASSERT_EQ(a, b)   do { if ((a) != (b)) { TEST_FAIL("Valeur attendue " + String(b) + " mais lu " + String(a)); return; } } while(0)

// ============================================
// FONCTIONS DE TEST
// ============================================

void testInit(FRAM_FM24CL16B &fram) {
    TEST_START("Initialisation I2C");
    if (fram.isInitialized()) {
        TEST_PASS();
    } else {
        TEST_FAIL("FRAM non detectee");
    }
}

void testBasicByte(FRAM_FM24CL16B &fram) {
    TEST_START("Ecriture/Lecture octet");

    uint8_t patterns[] = {0x00, 0xFF, 0x55, 0xAA, 0x12, 0x34, 0xDE, 0xAD};
    bool ok = true;

    for (size_t i = 0; i < sizeof(patterns); i++) {
        uint16_t addr = 0x0100 + i;
        fram.writeByte(addr, patterns[i]);
        uint8_t val = fram.readByte(addr);
        if (val != patterns[i]) {
            ok = false;
            break;
        }
    }

    if (ok) TEST_PASS();
    else      TEST_FAIL("Pattern octet incorrect");
}

void testCrossPageBoundary(FRAM_FM24CL16B &fram) {
    TEST_START("Frontiere de page (cross-page)");

    // La FM24CL16B a des pages de 256 octets
    // Adresses critiques : 0x00FF->0x0100, 0x01FF->0x0200, etc.
    uint16_t boundaries[] = {0x00FF, 0x0100, 0x01FF, 0x0200, 0x03FF, 0x0400, 0x05FF, 0x0600, 0x07FF};
    bool ok = true;

    for (size_t i = 0; i < sizeof(boundaries)/sizeof(boundaries[0]); i++) {
        uint16_t addr = boundaries[i];
        uint8_t expected = (uint8_t)(addr & 0xFF);
        fram.writeByte(addr, expected);
        uint8_t val = fram.readByte(addr);
        if (val != expected) {
            Serial.print("\n  Erreur @0x"); Serial.print(addr, HEX);
            Serial.print(" attendu 0x"); Serial.print(expected, HEX);
            Serial.print(" lu 0x"); Serial.print(val, HEX);
            ok = false;
            break;
        }
    }

    if (ok) TEST_PASS();
    else      TEST_FAIL("Erreur frontiere de page");
}

void testSequentialWriteRead(FRAM_FM24CL16B &fram) {
    TEST_START("Ecriture/Lecture sequentielle (256 octets)");

    static uint8_t writeBuf[256];
    static uint8_t readBuf[256];

    for (int i = 0; i < 256; i++) {
        writeBuf[i] = (uint8_t)(i ^ 0xA5);  // pattern pseudo-aleatoire
    }

    fram_error_t err = fram.write(0x0200, writeBuf, 256);
    if (err != FRAM_OK) {
        TEST_FAIL("Erreur ecriture bloc");
        return;
    }

    err = fram.read(0x0200, readBuf, 256);
    if (err != FRAM_OK) {
        TEST_FAIL("Erreur lecture bloc");
        return;
    }

    bool ok = true;
    for (int i = 0; i < 256; i++) {
        if (writeBuf[i] != readBuf[i]) {
            Serial.print("\n  Diff @"); Serial.print(i);
            Serial.print(" W="); Serial.print(writeBuf[i], HEX);
            Serial.print(" R="); Serial.print(readBuf[i], HEX);
            ok = false;
            break;
        }
    }

    if (ok) TEST_PASS();
    else      TEST_FAIL("Donnees bloc corrompues");
}

void testDataTypes(FRAM_FM24CL16B &fram) {
    TEST_START("Types de donnees varies");

    bool ok = true;

    // int16_t
    int16_t si = -12345;
    fram.writeInt(0x0300, si);
    int16_t si_r; fram.readInt(0x0300, &si_r);
    if (si != si_r) { ok = false; Serial.print("\n  int16 fail"); }

    // int32_t
    int32_t li = 123456789L;
    fram.writeLong(0x0304, li);
    int32_t li_r; fram.readLong(0x0304, &li_r);
    if (li != li_r) { ok = false; Serial.print("\n  int32 fail"); }

    // float
    float f = 3.14159265f;
    fram.writeFloat(0x0308, f);
    float f_r; fram.readFloat(0x0308, &f_r);
    if (abs(f - f_r) > 0.0001f) { ok = false; Serial.print("\n  float fail"); }

    // double
    double d = 2.718281828459045;
    fram.writeDouble(0x030C, d);
    double d_r; fram.readDouble(0x030C, &d_r);
    if (abs(d - d_r) > 0.0000001) { ok = false; Serial.print("\n  double fail"); }

    // struct
    struct TestStruct {
        uint8_t  a;
        uint16_t b;
        uint32_t c;
        float    d;
    };
    TestStruct ts = {0xAB, 0x1234, 0xDEADBEEF, 1.234f};
    fram.write(0x0320, &ts, sizeof(ts));
    TestStruct ts_r;
    fram.read(0x0320, &ts_r, sizeof(ts_r));
    if (ts.a != ts_r.a || ts.b != ts_r.b || ts.c != ts_r.c || abs(ts.d - ts_r.d) > 0.001f) {
        ok = false; Serial.print("\n  struct fail");
    }

    // string
    char strW[] = "Hello FRAM!";
    char strR[32] = {0};
    fram.write(0x0340, strW, strlen(strW) + 1);
    fram.read(0x0340, strR, sizeof(strR));
    if (strcmp(strW, strR) != 0) { ok = false; Serial.print("\n  string fail"); }

    if (ok) TEST_PASS();
    else      TEST_FAIL("Type de donnee incorrect");
}

void testAddressProtection(FRAM_FM24CL16B &fram) {
    TEST_START("Protection d'adresse (overflow)");

    uint8_t buf[16];
    fram_error_t err;
    bool ok = true;

    // Ecriture hors limites
    err = fram.write(0x07F0, buf, 32);   // 0x7F0 + 32 = 0x810 > 0x7FF
    if (err != FRAM_ERROR_ADDR_OVERFLOW) ok = false;

    // Adresse max + 1
    err = fram.write(0x0800, buf, 1);
    if (err != FRAM_ERROR_ADDR_OVERFLOW) ok = false;

    // Pointeur NULL
    err = fram.write(0x0100, nullptr, 10);
    if (err != FRAM_ERROR_NULL_POINTER) ok = false;

    // Longueur nulle
    err = fram.write(0x0100, buf, 0);
    if (err != FRAM_ERROR_ZERO_LENGTH) ok = false;

    // Ecriture exacte a la limite (doit passer)
    err = fram.write(0x07F0, buf, 16);   // 0x7F0 + 16 = 0x800 = OK
    if (err != FRAM_OK) ok = false;

    if (ok) TEST_PASS();
    else      TEST_FAIL("Protection d'adresse incorrecte");
}

void testClearFill(FRAM_FM24CL16B &fram) {
    TEST_START("Clear() et Fill()");

    bool ok = true;

    // Fill avec 0xAA
    fram.fill(0x0400, 32, 0xAA);
    for (int i = 0; i < 32; i++) {
        if (fram.readByte(0x0400 + i) != 0xAA) { ok = false; break; }
    }

    // Clear (fill avec 0x00)
    fram.clear(0x0400, 32);
    for (int i = 0; i < 32; i++) {
        if (fram.readByte(0x0400 + i) != 0x00) { ok = false; break; }
    }

    if (ok) TEST_PASS();
    else      TEST_FAIL("Clear/Fill incorrect");
}

void testPersistence(FRAM_FM24CL16B &fram) {
    TEST_START("Persistance (compteur de boot)");

    const uint32_t MAGIC = 0xDEADBEEF;
    const uint16_t ADR_MAGIC = 0x0500;
    const uint16_t ADR_COUNT = 0x0504;

    uint32_t magic = 0;
    fram.read(ADR_MAGIC, &magic, sizeof(magic));

    uint32_t bootCount = 0;
    if (magic == MAGIC) {
        fram.read(ADR_COUNT, &bootCount, sizeof(bootCount));
    } else {
        fram.write(ADR_MAGIC, &MAGIC, sizeof(MAGIC));
        bootCount = 0;
    }

    bootCount++;
    fram.write(ADR_COUNT, &bootCount, sizeof(bootCount));

    // Verification immediate
    uint32_t verify = 0;
    fram.read(ADR_COUNT, &verify, sizeof(verify));

    if (verify == bootCount) {
        Serial.print(" (boot #"); Serial.print(bootCount); Serial.print(") ");
        TEST_PASS();
    } else {
        TEST_FAIL("Persistance non verifiee");
    }
}

void testFullMemoryWalk(FRAM_FM24CL16B &fram) {
    TEST_START("Test complet memoire (walk pattern)");

    bool ok = true;
    uint16_t step = 64;  // tester tous les 64 octets pour aller vite

    for (uint16_t addr = 0; addr <= FRAM_MAX_ADDRESS; addr += step) {
        uint8_t expected = (uint8_t)((addr >> 2) ^ 0x5A);
        fram.writeByte(addr, expected);
        uint8_t val = fram.readByte(addr);
        if (val != expected) {
            Serial.print("\n  Erreur @0x"); Serial.print(addr, HEX);
            ok = false;
            break;
        }
    }

    if (ok) TEST_PASS();
    else      TEST_FAIL("Erreur lors du walk memoire");
}

void testStress(FRAM_FM24CL16B &fram) {
    TEST_START("Stress test (10 cycles ecriture/lecture)");

    static uint8_t bufW[64];
    static uint8_t bufR[64];
    bool ok = true;

    for (int cycle = 0; cycle < 10; cycle++) {
        for (int i = 0; i < 64; i++) {
            bufW[i] = (uint8_t)(cycle * 7 + i * 3);
        }
        fram.write(0x0600, bufW, 64);
        fram.read(0x0600, bufR, 64);
        for (int i = 0; i < 64; i++) {
            if (bufW[i] != bufR[i]) { ok = false; break; }
        }
        if (!ok) break;
    }

    if (ok) TEST_PASS();
    else      TEST_FAIL("Erreur stress test");
}

// ============================================
// RAPPORT FINAL
// ============================================
void printReport() {
    Serial.println("\n");
    Serial.println("==================================================");
    Serial.println("           RAPPORT DE TEST FRAM FM24CL16B");
    Serial.println("==================================================");
    Serial.print("  Tests total  : "); Serial.println(stats.total);
    Serial.print("  Tests reussis: "); Serial.println(stats.passed);
    Serial.print("  Tests echoues: "); Serial.println(stats.failed);
    Serial.println("--------------------------------------------------");
    if (stats.failed == 0) {
        Serial.println("  RESULTAT : TOUTES LES VERIFICATIONS ONT REUSSI");
        Serial.println("  La FRAM fonctionne correctement !");
    } else {
        Serial.println("  RESULTAT : CERTAINS TESTS ONT ECHOUE");
        Serial.println("  Verifiez le cablage ou remplacez la puce.");
    }
    Serial.println("==================================================");
}

// ============================================
// SETUP / LOOP
// ============================================

void setup() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial) delay(10);
    delay(500);

    Serial.println("\n");
    Serial.println("==================================================");
    Serial.println("     TEST COMPLET FRAM FM24CL16B v1.0");
    Serial.println("==================================================");
    Serial.println("Initialisation I2C...");

    Wire.begin();
    Wire.setClock(I2C_SPEED);

    FRAM_FM24CL16B fram;
    if (!fram.begin(I2C_SPEED)) {
        Serial.println("\nERREUR FATALE : FRAM non detectee sur le bus I2C !");
        Serial.println("Verifiez le cablage (SDA, SCL, VCC=3.3V, GND).");
        Serial.println("Arret du test.");
        return;
    }

    Serial.println("FRAM detectee !");
    printFRAMInfo(fram, Serial);

    // Execution des tests
    testInit(fram);
    testBasicByte(fram);
    testCrossPageBoundary(fram);
    testSequentialWriteRead(fram);
    testDataTypes(fram);
    testAddressProtection(fram);
    testClearFill(fram);
    testPersistence(fram);
    testFullMemoryWalk(fram);
    testStress(fram);

    // Rapport
    printReport();

    Serial.println("\nTest termine. Redemarrez pour relancer.");
}

void loop() {
    // Rien a faire
}
