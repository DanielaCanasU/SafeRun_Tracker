#pragma once
// Copiado desde Codigo_cursor/SX1262_Settings.h (ajusta si tus pines difieren en Heltec)

#include <SX126XLT.h>

#define DIO1 14
#define DIO2 -1
#define DIO3 -1
#define RX_EN -1
#define TX_EN -1
#define SW -1

#define SCK 9
#define MISO 11
#define MOSI 10

#define NSS 8
#define NRESET 12
#define RFBUSY 13

#define LED1 18

#define LORA_DEVICE DEVICE_SX1262

const uint32_t Frequency = 915000000;
const uint32_t Offset = 0;
const uint8_t Bandwidth = LORA_BW_125;
const uint8_t SpreadingFactor = LORA_SF12;
const uint8_t CodeRate = LORA_CR_4_8;
const uint8_t Optimisation = LDRO_AUTO;
const int8_t TXpower = 22;
const uint16_t packet_delay = 1000;

const uint16_t RXBUFFER_SIZE = 200;
