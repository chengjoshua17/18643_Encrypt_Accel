#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t state_t[4][4];

// Function Definitions
static uint8_t getSBoxValue(uint8_t n);
static void SubBytes(state_t* state);
static void ShiftRows(state_t* state);
static uint8_t xtime(uint8_t x);
static void MixColumns(state_t* state);
static void KeyExpansion(uint8_t* RoundKey, const uint8_t* Key);
static void AddRoundKey(uint8_t round, state_t* state, const uint8_t* RoundKey);
static void Encrypt_AES(state_t* state, const uint8_t* RoundKey);
void AESEncrypt(unsigned long data);
