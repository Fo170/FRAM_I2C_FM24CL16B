# FRAM_I2C_FM24CL16B

Bibliothèque Arduino pour la FRAM **Cypress FM24CL16B** 16-Kbit (2K x 8) sur bus I2C.

- Haute endurance : 10^14 lectures/écritures
- Rétention des données : 151 ans
- Interface I2C jusqu'à 1 MHz (configurable)
- Gestion multi-puces : jusqu'à 8 modules FM24CL16B (16 Ko total)

## Fonctionnalités

- Écriture/lecture par blocs avec découpage automatique aux limites de page (256 octets)
- Gestion transparente de l'adressage multi-puces (A2, A1, A0)
- Offset d'adresse de base (`_baseAddress`) pour espace d'adressage contigu
- Classe `FRAM_Manager` pour piloter jusqu'à 8 puces dans un espace d'adressage global unique
- Appels `yield()` pour compatibilité ESP8266/ESP32
- Fonctions utilitaires : `clear()`, `fill()`, `dump()` hex+ASCII
- Retours d'erreur typés (`fram_error_t`)

## Installation

### PlatformIO

Ajoutez à votre `platformio.ini` :

```ini
lib_deps =
    https://github.com/Fo170/FRAM_I2C_FM24CL16B.git
```

Ou placez le dossier dans `lib/` de votre projet.

### Arduino IDE

Téléchargez le dépôt ou installez-le via Gestionnaire de bibliothèques (à venir). Placez `FRAM_I2C_FM24CL16B.h` dans le dossier `libraries/` de votre sketchbook.

## Brochage

```
FM24CL16B         Arduino
--------         -------
VCC      ->      3.3V / 5V
GND      ->      GND
SCL      ->      A5 (Uno) / D22 (Mega) / GPIO5 (ESP8266) / GPIO22 (ESP32)
SDA      ->      A4 (Uno) / D20 (Mega) / GPIO4 (ESP8266) / GPIO21 (ESP32)
WP       ->      GND (write enabled)
A0/A1/A2 ->      GND ou VCC (adresse I2C, voir ci-dessous)
```

### Adressage multi-puces

Jusqu'à 8 puces FM24CL16B peuvent partager le bus I2C. Les broches A0, A1, A2 définissent l'adresse physique de chaque puce :

| A2 A1 A0 | Adresse I2C | Plage mémoire |
|-----------|-------------|---------------|
| 0 0 0     | 0x50        | 0x000 - 0x7FF |
| 0 0 1     | 0x51        | 0x800 - 0xFFF |
| 0 1 0     | 0x52        | 0x1000 - 0x17FF |
| 0 1 1     | 0x53        | 0x1800 - 0x1FFF |
| 1 0 0     | 0x54        | 0x2000 - 0x27FF |
| 1 0 1     | 0x55        | 0x2800 - 0x2FFF |
| 1 1 0     | 0x56        | 0x3000 - 0x37FF |
| 1 1 1     | 0x57        | 0x3800 - 0x3FFF |

## API

### FRAM_FM24CL16B

```cpp
#include <FRAM_I2C_FM24CL16B.h>
```

#### Constructeurs

```cpp
FRAM_FM24CL16B();                                  // Adresse 0x50
FRAM_FM24CL16B(fram_address_config_t addrConfig);  // FRAM_ADDR_000 à FRAM_ADDR_111
FRAM_FM24CL16B(uint8_t i2cAddr);                   // Adresse brute
FRAM_FM24CL16B(fram_address_config_t addrConfig, uint16_t baseAddr); // Avec offset
```

#### Initialisation

```cpp
bool begin();                    // Wire.begin() + détection
bool begin(uint32_t clockSpeed); // Avec fréquence I2C personnalisée
```

Retourne `true` si la puce est détectée.

#### Lecture / Écriture

```cpp
fram_error_t write(uint16_t address, const void *data, uint16_t length);
fram_error_t read(uint16_t address, void *data, uint16_t length);
```

Types helpers :

```cpp
fram_error_t writeByte(uint16_t addr, uint8_t value);
fram_error_t writeInt(uint16_t addr, int16_t value);
fram_error_t writeLong(uint16_t addr, int32_t value);
fram_error_t writeFloat(uint16_t addr, float value);
fram_error_t writeDouble(uint16_t addr, double value);

fram_error_t readByte(uint16_t addr, uint8_t *value);
uint8_t      readByte(uint16_t addr); // version retour directe
fram_error_t readInt(uint16_t addr, int16_t *value);
fram_error_t readLong(uint16_t addr, int32_t *value);
fram_error_t readFloat(uint16_t addr, float *value);
fram_error_t readDouble(uint16_t addr, double *value);
```

#### Effacement et remplissage

```cpp
fram_error_t clear(uint16_t startAddr, uint16_t length);
fram_error_t clearAll();       // efface toute la puce
fram_error_t fill(uint16_t startAddr, uint16_t length, uint8_t value);
```

#### Diagnostic

```cpp
void dump(uint16_t startAddr, uint16_t length, Stream &output = Serial);
void dumpAll(Stream &output = Serial);
```

#### Gestion

```cpp
bool checkDevice();
bool checkDeviceAt(uint16_t memAddr);
bool isInitialized();
uint8_t getDeviceAddress();
uint16_t getBaseAddress();
uint16_t getSize();      // 2048
uint16_t getMaxAddr();   // 0x7FF
```

### FRAM_Manager (multi-puces)

```cpp
FRAM_Manager manager;

bool addDevice(fram_address_config_t addrConfig);
bool addDevice(fram_address_config_t addrConfig, uint16_t globalOffset);

uint8_t  getDeviceCount();
uint16_t getTotalSize();
FRAM_FM24CL16B* getDevice(uint8_t index);

fram_error_t writeGlobal(uint16_t globalAddr, const void *data, uint16_t len);
fram_error_t readGlobal(uint16_t globalAddr, void *data, uint16_t len);
```

### Codes d'erreur

```cpp
FRAM_OK                  = 0
FRAM_ERROR_ADDR_OVERFLOW    Dépassement d'adresse mémoire
FRAM_ERROR_NULL_POINTER     Pointeur nul
FRAM_ERROR_ZERO_LENGTH      Longueur nulle
FRAM_ERROR_DEVICE_NOT_FOUND Puce non détectée
FRAM_ERROR_I2C_ERROR        Erreur de transmission I2C
FRAM_ERROR_INVALID_DEVICE   Puce invalide
FRAM_ERROR_BUSY             Puce occupée
```

## Exemple

Un sketch de test complet est disponible dans [`examples/fram_test/`](examples/fram_test/fram_test.ino). Il couvre :

- Initialisation et détection
- `writeByte` / `readByte`, `writeInt` / `readInt`, `writeLong` / `readLong`
- `writeFloat` / `readFloat`, `writeDouble` / `readDouble`
- `write` / `read` par blocs
- `clear()`, `fill()`, `clearAll()`
- Gestion des erreurs (overflow, pointeur nul, longueur nulle)
- `readByte()` en retour direct
- `dump()` hex+ASCII
- `FRAM_Manager` multi-puces

## Compatibilité

Testé ou compatible avec :

- Arduino Uno / Mega / Nano
- ESP8266 (NodeMCU, Wemos D1)
- ESP32 / ESP32-S3
- Teensy 3.x / 4.x
- STM32 (via Arduino Core)

## Version

**v1.0.0** — Version initiale : type `uint16_t` pour `pageBoundary` (évite overflow), gestion des lectures cross-page, compatibilité `_baseAddress` dans `clear()`/`fill()`/`dump()`, ajout de `yield()` pour ESP8266.

## Licence

GPL-3.0
