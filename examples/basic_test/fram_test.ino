/**
 * FRAM_I2C_FM24CL16B — Test complet
 *
 * Teste les fonctionnalités de base : écriture/lecture,
 * effacement, remplissage, dump et multi-puces.
 *
 * Brochage I2C par défaut (à adapter à votre carte) :
 *   Arduino Uno : SDA=A4, SCL=A5
 *   ESP32       : SDA=GPIO21, SCL=GPIO22
 *   ESP8266     : SDA=GPIO4,  SCL=GPIO5
 */

#include <Arduino.h>
#include <FRAM_I2C_FM24CL16B.h>

FRAM_FM24CL16B fram;
uint16_t errors = 0;
uint16_t tests  = 0;

void check(const char *label, fram_error_t err, fram_error_t expected = FRAM_OK) {
    tests++;
    if (err != expected) {
        errors++;
        Serial.print("  [ECHEC] ");
        Serial.print(label);
        Serial.print(" -> ");
        printError(err, Serial);
    } else {
        Serial.print("  [OK] ");
        Serial.println(label);
    }
}

void checkBool(const char *label, bool result, bool expected = true) {
    tests++;
    if (result != expected) {
        errors++;
        Serial.print("  [ECHEC] ");
        Serial.println(label);
    } else {
        Serial.print("  [OK] ");
        Serial.println(label);
    }
}

void checkBuffer(const char *label, const uint8_t *buf, uint8_t val, uint16_t len) {
    tests++;
    for (uint16_t i = 0; i < len; i++) {
        if (buf[i] != val) {
            errors++;
            Serial.print("  [ECHEC] ");
            Serial.print(label);
            Serial.print(" offset ");
            Serial.print(i);
            Serial.print(": attendu 0x");
            Serial.print(val, HEX);
            Serial.print(", lu 0x");
            Serial.println(buf[i], HEX);
            return;
        }
    }
    Serial.print("  [OK] ");
    Serial.println(label);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println("=== FRAM_I2C_FM24CL16B Tests ===");
    Serial.println();

    // --- Test 1 : Initialisation ---
    Serial.println("--- init ---");
    checkBool("begin()", fram.begin());

    // --- Test 2 : Info ---
    Serial.println("--- info ---");
    printFRAMInfo(fram, Serial);

    // --- Test 3 : Écriture / lecture byte ---
    Serial.println("--- writeByte / readByte ---");
    check("writeByte(0, 0x42)", fram.writeByte(0, 0x42));
    uint8_t val;
    check("readByte(0, &val)", fram.readByte(0, &val));
    checkBool("val == 0x42", val == 0x42);

    // --- Test 4 : write / read multi-octets ---
    Serial.println("--- write / read blocs ---");
    const uint8_t src[] = {0x10, 0x20, 0x30, 0x40, 0x50};
    uint8_t dst[5] = {0};
    check("write(100, src, 5)", fram.write(100, src, 5));
    check("read(100, dst, 5)", fram.read(100, dst, 5));
    checkBool("dst[0]==0x10", dst[0] == 0x10);
    checkBool("dst[4]==0x50", dst[4] == 0x50);

    // --- Test 5 : writeInt / readInt ---
    Serial.println("--- writeInt / readInt ---");
    int16_t iVal = -1234;
    check("writeInt(200, -1234)", fram.writeInt(200, iVal));
    int16_t iOut;
    check("readInt(200, &iOut)", fram.readInt(200, &iOut));
    checkBool("iOut == -1234", iOut == -1234);

    // --- Test 6 : writeLong / readLong ---
    Serial.println("--- writeLong / readLong ---");
    int32_t lVal = 12345678;
    check("writeLong(300, 12345678)", fram.writeLong(300, lVal));
    int32_t lOut;
    check("readLong(300, &lOut)", fram.readLong(300, &lOut));
    checkBool("lOut == 12345678", lOut == 12345678);

    // --- Test 7 : writeFloat / readFloat ---
    Serial.println("--- writeFloat / readFloat ---");
    float fVal = 3.14159f;
    check("writeFloat(400, 3.14159)", fram.writeFloat(400, fVal));
    float fOut;
    check("readFloat(400, &fOut)", fram.readFloat(400, &fOut));
    checkBool("fOut ~= 3.14159", abs(fOut - 3.14159f) < 0.001f);

    // --- Test 8 : clear / fill ---
    Serial.println("--- clear / fill ---");
    uint8_t buf[16];

    check("fill(0, 16, 0xAA)", fram.fill(0, 16, 0xAA));
    check("read(0, buf, 16)", fram.read(0, buf, 16));
    checkBuffer("buffer contient 0xAA", buf, 0xAA, 16);

    check("clear(0, 16)", fram.clear(0, 16));
    check("read(0, buf, 16)", fram.read(0, buf, 16));
    checkBuffer("buffer contient 0x00", buf, 0x00, 16);

    // --- Test 9 : clearAll ---
    Serial.println("--- clearAll ---");
    check("clearAll()", fram.clearAll());
    check("readByte(0, &val)", fram.readByte(0, &val));
    checkBool("val == 0", val == 0);

    // --- Test 10 : Lecture hors limites ---
    Serial.println("--- overflow ---");
    check("write(2048, src, 1)", fram.write(2048, src, 1), FRAM_ERROR_ADDR_OVERFLOW);
    check("read(2048, dst, 1)", fram.read(2048, dst, 1), FRAM_ERROR_ADDR_OVERFLOW);
    check("write(0, NULL, 1)", fram.write(0, nullptr, 1), FRAM_ERROR_NULL_POINTER);
    check("read(0, NULL, 1)", fram.read(0, nullptr, 1), FRAM_ERROR_NULL_POINTER);
    check("write(0, src, 0)", fram.write(0, src, 0), FRAM_ERROR_ZERO_LENGTH);
    check("read(0, dst, 0)", fram.read(0, dst, 0), FRAM_ERROR_ZERO_LENGTH);

    // --- Test 11 : readByte retour direct ---
    Serial.println("--- readByte direct ---");
    fram.writeByte(50, 0xBB);
    uint8_t v = fram.readByte(50);
    checkBool("readByte(50) == 0xBB", v == 0xBB);

    // --- Test 12 : Dump ---
    Serial.println("--- dump ---");
    fram.fill(0, 32, 0x55);
    fram.dump(0, 32);

    // --- Test 13 : writeDouble / readDouble ---
    Serial.println("--- writeDouble / readDouble ---");
    double dVal = 2.718281828;
    check("writeDouble(500, 2.71828)", fram.writeDouble(500, dVal));
    double dOut;
    check("readDouble(500, &dOut)", fram.readDouble(500, &dOut));
    checkBool("dOut ~= 2.71828", abs(dOut - 2.718281828) < 0.0001);

    // --- Test 14 : checkDeviceAt ---
    Serial.println("--- checkDeviceAt ---");
    checkBool("checkDeviceAt(0)", fram.checkDeviceAt(0));
    checkBool("checkDeviceAt(0x7FF)", fram.checkDeviceAt(0x7FF));

    // --- Test 15 : Multi-puces (FRAM_Manager) ---
    Serial.println("--- FRAM_Manager ---");
    FRAM_Manager manager;
    uint8_t added = 0;
    if (manager.addDevice(FRAM_ADDR_000)) added++;
    if (manager.addDevice(FRAM_ADDR_001)) added++;
    Serial.print("  Puces detectees : ");
    Serial.println(manager.getDeviceCount());
    if (added > 0) {
        checkBool("writeGlobal(0, src, 5)", manager.writeGlobal(0, src, 5) == FRAM_OK);
        checkBool("readGlobal(0, dst, 5)", manager.readGlobal(0, dst, 5) == FRAM_OK);
        checkBool("global dst[0]==0x10", dst[0] == 0x10);
    }

    // --- Résultat ---
    Serial.println();
    Serial.println("=== Résultat ===");
    Serial.print("Tests: ");
    Serial.print(tests);
    Serial.print(", Echecs: ");
    Serial.println(errors);

    if (errors == 0) {
        Serial.println("TOUS LES TESTS ONT REUSSI");
    } else {
        Serial.println("CERTAINS TESTS ONT ECHOUE");
    }
}

void loop() {}
