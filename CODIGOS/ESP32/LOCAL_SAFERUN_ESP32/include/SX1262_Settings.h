#ifndef SX1262_SETTINGS_H
#define SX1262_SETTINGS_H

// =============================================================================
// SX1262 LORA CONFIGURATION
// =============================================================================

// Pin definitions for SX1262
#define NSS 5
#define NRESET 14
#define RFBUSY 27
#define DIO1 26
#define LORA_DEVICE DEVICE_SX1262

// Frequency settings
#define Frequency 915000000
#define Offset 0

// LoRa modulation parameters
#define SpreadingFactor LORA_SF7
#define Bandwidth LORA_BW_125
#define CodeRate LORA_CR_4_5
#define Optimisation LDRO_AUTO

// Sync word
#define LORA_MAC_PRIVATE_SYNCWORD 0x12

#endif // SX1262_SETTINGS_H 