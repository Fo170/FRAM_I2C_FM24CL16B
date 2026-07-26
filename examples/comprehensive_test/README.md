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

