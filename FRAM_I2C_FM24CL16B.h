// FRAM en I2C des FM24CL16B de Cypress 16-Kbit (2K x 8)
// High-endurance 100 trillion (10^14) read/writes
// 151-year data retention 
// https://www.mouser.fr/datasheet/2/100/CYPR_S_A0010744052_1-2541108.pdf

// Gestion multi-puces FM24CL16B avec protection memoire
// Cypress 16-Kbit (2K x 8) - High-endurance 10^14 read/writes, 151-year retention

// FRAM_I2C_FM24CL16B.h v2.4
// Corrections: 
//  - uint16_t pageBoundary dans write/read pour eviter boucle infinie (overflow uint8_t)
//  - gestion pageBoundary dans read() pour lectures cross-page
//  - adressage physique dans clear/fill/dump pour compatibilite _baseAddress
//  - yield() ajoute dans les boucles pour ESP8266

#ifndef FRAM_I2C_FM24CL16B_H
#define FRAM_I2C_FM24CL16B_H

#include <Wire.h>
#include <Stream.h>

// ============================================
// CONFIGURATION MATERIELLE ET TYPES
// ============================================

#define FRAM_BASE_ADDRESS       0x50
#define FRAM_SIZE_BYTES         2048
#define FRAM_SIZE_BITS          16384
#define FRAM_MAX_ADDRESS        0x7FF

typedef enum {
    FRAM_ADDR_000 = 0x00,
    FRAM_ADDR_001 = 0x01,
    FRAM_ADDR_010 = 0x02,
    FRAM_ADDR_011 = 0x03,
    FRAM_ADDR_100 = 0x04,
    FRAM_ADDR_101 = 0x05,
    FRAM_ADDR_110 = 0x06,
    FRAM_ADDR_111 = 0x07
} fram_address_config_t;

typedef enum {
    FRAM_OK = 0,
    FRAM_ERROR_ADDR_OVERFLOW,
    FRAM_ERROR_NULL_POINTER,
    FRAM_ERROR_ZERO_LENGTH,
    FRAM_ERROR_DEVICE_NOT_FOUND,
    FRAM_ERROR_I2C_ERROR,
    FRAM_ERROR_INVALID_DEVICE,
    FRAM_ERROR_BUSY
} fram_error_t;

// ============================================
// DECLARATION ANTICIPEE CLASSE
// ============================================

class FRAM_FM24CL16B;

// ============================================
// FONCTIONS UTILITAIRES - DECLARATIONS
// ============================================

void printError(fram_error_t err, Stream &output);
void printFRAMInfo(FRAM_FM24CL16B &fram, Stream &output);

// ============================================
// CLASSE PRINCIPALE
// ============================================

class FRAM_FM24CL16B {
private:
    uint8_t _deviceAddr;
    uint16_t _baseAddress;
    bool _initialized;

    uint8_t _getDeviceAddress(uint16_t memAddr) {
        uint8_t pageSelect = (memAddr >> 8) & 0x07;
        return _deviceAddr | pageSelect;
    }

    bool _checkBounds(uint16_t startAddr, uint16_t length, uint16_t &endAddr) {
        if (length == 0) return false;
        if (startAddr > FRAM_MAX_ADDRESS) return false;

        endAddr = startAddr + length - 1;
        if (endAddr > FRAM_MAX_ADDRESS || endAddr < startAddr) {
            return false;
        }
        return true;
    }

public:
    FRAM_FM24CL16B() : _deviceAddr(FRAM_BASE_ADDRESS), _baseAddress(0x0000), _initialized(false) {}

    FRAM_FM24CL16B(fram_address_config_t addrConfig) : 
        _deviceAddr(FRAM_BASE_ADDRESS | (addrConfig & 0x07)), 
        _baseAddress(0x0000), 
        _initialized(false) {}

    FRAM_FM24CL16B(uint8_t i2cAddr) : 
        _deviceAddr(i2cAddr & 0x57), 
        _baseAddress(0x0000), 
        _initialized(false) {}

    FRAM_FM24CL16B(fram_address_config_t addrConfig, uint16_t baseAddr) : 
        _deviceAddr(FRAM_BASE_ADDRESS | (addrConfig & 0x07)), 
        _baseAddress(baseAddr), 
        _initialized(false) {}

    bool begin() {
        Wire.begin();
        _initialized = checkDevice();
        return _initialized;
    }

    bool begin(uint32_t clockSpeed) {
        Wire.begin();
        Wire.setClock(clockSpeed);
        _initialized = checkDevice();
        return _initialized;
    }

    bool checkDevice() {
        Wire.beginTransmission(_deviceAddr);
        return (Wire.endTransmission(true) == 0);
    }

    bool checkDeviceAt(uint16_t memAddr) {
        uint8_t addr = _getDeviceAddress(memAddr);
        Wire.beginTransmission(addr);
        return (Wire.endTransmission(true) == 0);
    }

    bool isInitialized() { return _initialized; }
    uint8_t getDeviceAddress() { return _deviceAddr; }
    uint16_t getBaseAddress() { return _baseAddress; }

    fram_error_t write(uint16_t startAddress, const void *data, uint16_t len) {
        if (data == nullptr) return FRAM_ERROR_NULL_POINTER;
        if (len == 0) return FRAM_ERROR_ZERO_LENGTH;

        uint16_t endAddr;
        uint16_t physicalAddr = _baseAddress + startAddress;

        if (!_checkBounds(physicalAddr, len, endAddr)) {
            return FRAM_ERROR_ADDR_OVERFLOW;
        }

        const uint8_t *p = (const uint8_t*)data;
        uint16_t remaining = len;
        uint16_t currentAddr = physicalAddr;

        while (remaining > 0) {
            uint8_t devAddr = _getDeviceAddress(currentAddr);
            uint8_t addrLow = currentAddr & 0xFF;

            Wire.beginTransmission(devAddr);
            Wire.write(addrLow);

            uint8_t bytesInPage = (remaining > 16) ? 16 : (uint8_t)remaining;
            uint16_t pageBoundary = 256u - (currentAddr & 0xFFu);
            bytesInPage = (bytesInPage > pageBoundary) ? (uint8_t)pageBoundary : bytesInPage;

            for (uint8_t i = 0; i < bytesInPage; i++) {
                Wire.write(*p++);
            }

            if (Wire.endTransmission(true) != 0) {
                return FRAM_ERROR_I2C_ERROR;
            }

            remaining -= bytesInPage;
            currentAddr += bytesInPage;

            if (remaining > 0) {
                delayMicroseconds(250);
                yield();
            }
        }

        return FRAM_OK;
    }

    fram_error_t read(uint16_t startAddress, void *data, uint16_t len) {
        if (data == nullptr) return FRAM_ERROR_NULL_POINTER;
        if (len == 0) return FRAM_ERROR_ZERO_LENGTH;

        uint16_t endAddr;
        uint16_t physicalAddr = _baseAddress + startAddress;

        if (!_checkBounds(physicalAddr, len, endAddr)) {
            return FRAM_ERROR_ADDR_OVERFLOW;
        }

        uint8_t *p = (uint8_t*)data;
        uint16_t remaining = len;
        uint16_t currentAddr = physicalAddr;

        while (remaining > 0) {
            uint8_t devAddr = _getDeviceAddress(currentAddr);
            uint8_t addrLow = currentAddr & 0xFF;

            Wire.beginTransmission(devAddr);
            Wire.write(addrLow);
            if (Wire.endTransmission(false) != 0) {
                return FRAM_ERROR_I2C_ERROR;
            }

            uint8_t bytesToRead = (remaining > 32) ? 32 : (uint8_t)remaining;
            uint16_t pageBoundary = 256u - (currentAddr & 0xFFu);
            bytesToRead = (bytesToRead > pageBoundary) ? (uint8_t)pageBoundary : bytesToRead;

            uint8_t requested = Wire.requestFrom((int)devAddr, (int)bytesToRead, (int)true);

            if (requested != bytesToRead) {
                return FRAM_ERROR_I2C_ERROR;
            }

            for (uint8_t i = 0; i < bytesToRead; i++) {
                if (Wire.available()) {
                    *p++ = Wire.read();
                } else {
                    return FRAM_ERROR_I2C_ERROR;
                }
            }

            remaining -= bytesToRead;
            currentAddr += bytesToRead;

            yield();
        }

        return FRAM_OK;
    }

    fram_error_t writeByte(uint16_t addr, uint8_t value) {
        return write(addr, &value, sizeof(value));
    }

    fram_error_t readByte(uint16_t addr, uint8_t *value) {
        return read(addr, value, sizeof(*value));
    }

    uint8_t readByte(uint16_t addr) {
        uint8_t val = 0;
        read(addr, &val, sizeof(val));
        return val;
    }

    fram_error_t writeInt(uint16_t addr, int16_t value) {
        return write(addr, &value, sizeof(value));
    }

    fram_error_t readInt(uint16_t addr, int16_t *value) {
        return read(addr, value, sizeof(*value));
    }

    fram_error_t writeLong(uint16_t addr, int32_t value) {
        return write(addr, &value, sizeof(value));
    }

    fram_error_t readLong(uint16_t addr, int32_t *value) {
        return read(addr, value, sizeof(*value));
    }

    fram_error_t writeFloat(uint16_t addr, float value) {
        return write(addr, &value, sizeof(value));
    }

    fram_error_t readFloat(uint16_t addr, float *value) {
        return read(addr, value, sizeof(*value));
    }

    fram_error_t writeDouble(uint16_t addr, double value) {
        return write(addr, &value, sizeof(value));
    }

    fram_error_t readDouble(uint16_t addr, double *value) {
        return read(addr, value, sizeof(*value));
    }

    fram_error_t clear(uint16_t startAddr, uint16_t len) {
        uint16_t endAddr;
        uint16_t physicalAddr = _baseAddress + startAddr;

        if (!_checkBounds(physicalAddr, len, endAddr)) {
            return FRAM_ERROR_ADDR_OVERFLOW;
        }

        uint8_t zeroBlock[16] = {0};
        uint16_t remaining = len;
        uint16_t currentAddr = physicalAddr;

        while (remaining > 0) {
            uint8_t chunkSize = (remaining > 16) ? 16 : (uint8_t)remaining;
            uint16_t relativeAddr = currentAddr - _baseAddress;
            fram_error_t err = write(relativeAddr, zeroBlock, chunkSize);
            if (err != FRAM_OK) return err;
            currentAddr += chunkSize;
            remaining -= chunkSize;
        }
        return FRAM_OK;
    }

    fram_error_t clearAll() {
        return clear(0, FRAM_SIZE_BYTES);
    }

    fram_error_t fill(uint16_t startAddr, uint16_t len, uint8_t value) {
        uint16_t endAddr;
        uint16_t physicalAddr = _baseAddress + startAddr;

        if (!_checkBounds(physicalAddr, len, endAddr)) {
            return FRAM_ERROR_ADDR_OVERFLOW;
        }

        uint8_t block[16];
        memset(block, value, 16);

        uint16_t remaining = len;
        uint16_t currentAddr = physicalAddr;

        while (remaining > 0) {
            uint8_t chunkSize = (remaining > 16) ? 16 : (uint8_t)remaining;
            uint16_t relativeAddr = currentAddr - _baseAddress;
            fram_error_t err = write(relativeAddr, block, chunkSize);
            if (err != FRAM_OK) return err;
            currentAddr += chunkSize;
            remaining -= chunkSize;
        }
        return FRAM_OK;
    }

    void dump(uint16_t startAddr, uint16_t len, Stream &output = Serial) {
        uint16_t endAddr;
        uint16_t physicalAddr = _baseAddress + startAddr;

        if (!_checkBounds(physicalAddr, len, endAddr)) {
            output.println("ERROR: Address overflow");
            return;
        }

        uint8_t buffer[16];
        uint16_t currentAddr = physicalAddr;
        uint16_t remaining = len;

        while (remaining > 0) {
            uint8_t chunkSize = (remaining > 16) ? 16 : (uint8_t)remaining;
            uint16_t relativeAddr = currentAddr - _baseAddress;
            fram_error_t err = read(relativeAddr, buffer, chunkSize);

            if (err != FRAM_OK) {
                output.print("ERROR reading at 0x");
                output.println(currentAddr, HEX);
                return;
            }

            output.print("0x");
            if (currentAddr < 0x1000) output.print('0');
            output.print(currentAddr, HEX);
            output.print(": ");

            for (uint8_t i = 0; i < 16; i++) {
                if (i < chunkSize) {
                    if (buffer[i] < 0x10) output.print('0');
                    output.print(buffer[i], HEX);
                } else {
                    output.print("  ");
                }
                output.print(' ');
            }

            output.print(" | ");

            for (uint8_t i = 0; i < chunkSize; i++) {
                char c = buffer[i];
                output.print(isPrintable(c) ? c : '.');
            }

            output.println();
            currentAddr += chunkSize;
            remaining -= chunkSize;
        }
    }

    void dumpAll(Stream &output = Serial) {
        dump(0, FRAM_SIZE_BYTES, output);
    }

    uint16_t getSize() { return FRAM_SIZE_BYTES; }
    uint16_t getMaxAddr() { return FRAM_MAX_ADDRESS; }
};

// ============================================
// DEFINITIONS FONCTIONS UTILITAIRES (inline)
// ============================================

inline void printError(fram_error_t err, Stream &output) {
    output.print("Error: ");
    switch(err) {
        case FRAM_OK: output.println("OK"); break;
        case FRAM_ERROR_ADDR_OVERFLOW: output.println("ADDRESS OVERFLOW"); break;
        case FRAM_ERROR_NULL_POINTER: output.println("NULL POINTER"); break;
        case FRAM_ERROR_ZERO_LENGTH: output.println("ZERO LENGTH"); break;
        case FRAM_ERROR_DEVICE_NOT_FOUND: output.println("DEVICE NOT FOUND"); break;
        case FRAM_ERROR_I2C_ERROR: output.println("I2C ERROR"); break;
        case FRAM_ERROR_INVALID_DEVICE: output.println("INVALID DEVICE"); break;
        case FRAM_ERROR_BUSY: output.println("BUSY"); break;
        default: output.println("UNKNOWN"); break;
    }
}

inline void printFRAMInfo(FRAM_FM24CL16B &fram, Stream &output) {
    output.println("=== FRAM Info ===");
    output.print("I2C Address: 0x");
    output.println(fram.getDeviceAddress(), HEX);
    output.print("Size: ");
    output.print(fram.getSize());
    output.println(" bytes");
    output.print("Max Address: 0x");
    output.println(fram.getMaxAddr(), HEX);
    output.print("Initialized: ");
    output.println(fram.isInitialized() ? "YES" : "NO");
    output.println("=================");
}

// ============================================
// MANAGER MULTI-PUCES
// ============================================

class FRAM_Manager {
private:
    FRAM_FM24CL16B* _devices[8];
    uint8_t _deviceCount;
    uint16_t _totalSize;

public:
    FRAM_Manager() : _deviceCount(0), _totalSize(0) {
        for (int i = 0; i < 8; i++) _devices[i] = nullptr;
    }

    ~FRAM_Manager() {
        for (int i = 0; i < 8; i++) {
            if (_devices[i] != nullptr) {
                delete _devices[i];
                _devices[i] = nullptr;
            }
        }
    }

    bool addDevice(fram_address_config_t addrConfig) {
        if (_deviceCount >= 8) return false;

        FRAM_FM24CL16B* fram = new FRAM_FM24CL16B(addrConfig);
        if (!fram->begin()) {
            delete fram;
            return false;
        }

        _devices[_deviceCount] = fram;
        _deviceCount++;
        _totalSize += FRAM_SIZE_BYTES;
        return true;
    }

    bool addDevice(fram_address_config_t addrConfig, uint16_t globalOffset) {
        if (_deviceCount >= 8) return false;

        FRAM_FM24CL16B* fram = new FRAM_FM24CL16B(addrConfig, globalOffset);
        if (!fram->begin()) {
            delete fram;
            return false;
        }

        _devices[_deviceCount] = fram;
        _deviceCount++;
        _totalSize += FRAM_SIZE_BYTES;
        return true;
    }

    uint8_t getDeviceCount() { return _deviceCount; }
    uint16_t getTotalSize() { return _totalSize; }

    FRAM_FM24CL16B* getDevice(uint8_t index) {
        return (index < _deviceCount) ? _devices[index] : nullptr;
    }

    fram_error_t writeGlobal(uint16_t globalAddr, const void *data, uint16_t len) {
        if (globalAddr + len > _totalSize) return FRAM_ERROR_ADDR_OVERFLOW;

        uint16_t deviceStart = 0;
        for (uint8_t i = 0; i < _deviceCount; i++) {
            uint16_t deviceEnd = deviceStart + FRAM_SIZE_BYTES;

            if (globalAddr >= deviceStart && globalAddr < deviceEnd) {
                uint16_t localAddr = globalAddr - deviceStart;
                uint16_t available = deviceEnd - globalAddr;
                uint16_t writeLen = (len > available) ? available : len;

                fram_error_t err = _devices[i]->write(localAddr, data, writeLen);
                if (err != FRAM_OK) return err;

                if (writeLen < len) {
                    return writeGlobal(globalAddr + writeLen, 
                                     (const uint8_t*)data + writeLen, 
                                     len - writeLen);
                }
                return FRAM_OK;
            }
            deviceStart = deviceEnd;
        }
        return FRAM_ERROR_ADDR_OVERFLOW;
    }

    fram_error_t readGlobal(uint16_t globalAddr, void *data, uint16_t len) {
        if (globalAddr + len > _totalSize) return FRAM_ERROR_ADDR_OVERFLOW;

        uint16_t deviceStart = 0;
        for (uint8_t i = 0; i < _deviceCount; i++) {
            uint16_t deviceEnd = deviceStart + FRAM_SIZE_BYTES;

            if (globalAddr >= deviceStart && globalAddr < deviceEnd) {
                uint16_t localAddr = globalAddr - deviceStart;
                uint16_t available = deviceEnd - globalAddr;
                uint16_t readLen = (len > available) ? available : len;

                fram_error_t err = _devices[i]->read(localAddr, data, readLen);
                if (err != FRAM_OK) return err;

                if (readLen < len) {
                    return readGlobal(globalAddr + readLen,
                                    (uint8_t*)data + readLen,
                                    len - readLen);
                }
                return FRAM_OK;
            }
            deviceStart = deviceEnd;
        }
        return FRAM_ERROR_ADDR_OVERFLOW;
    }
};

#endif // FRAM_I2C_FM24CL16B_H
