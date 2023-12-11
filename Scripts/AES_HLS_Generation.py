# We needed to do terrible things in HLS to get AES encryptions to occur in parallel
# This is a script that performed those horrible things

repetitions = 24

f = open("krnl_cnn_layer1.cpp", "w")

start_string = """#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "krnl_cnn_layer1.h"

// Prevent aliasing
#undef BATCH_SIZE

#include "util643.h"

typedef uint8_t state_t[4][4];

// Round constant word array
const uint8_t Rcon[11] = {0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

// AES S-Box
"""

f.write(start_string)

s_box_string = """const uint8_t sbox{idx}[256] = {{
0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,   // 0
0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,   // 1
0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,   // 2
0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,   // 3
0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,   // 4
0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,   // 5
0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,   // 6
0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,   // 7
0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,   // 8
0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,   // 9
0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,   // A
0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,   // B
0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,   // C
0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,   // D
0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,   // E
0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16}};  // F

"""

for i in range(repetitions):
    f.write(s_box_string.format(idx = str(i)))

f.write("void KeyExpansion(uint8_t* RoundKey, const uint8_t* Key);\n\n")

fn_definition_string = """// Copy {idx}
uint8_t getSBoxValue{idx}(uint8_t n);
uint8_t xtime{idx}(uint8_t x);
void MixColumns{idx}(state_t* state);
void ShiftRows{idx}(state_t* state);
void SubBytes{idx}(state_t* state);
void AddRoundKey{idx}(uint8_t round, state_t* state, const uint8_t* RoundKey);
void Encrypt_AES{idx}(state_t* state, const uint8_t* RoundKey);
"""

for i in range(repetitions):
    f.write(fn_definition_string.format(idx = str(i)))

next_string = """
#ifdef __VITIS_CL__
extern "C" {{
#endif

// AES Implementation
void krnl_cnn_layer1(unsigned long *pt_ptr, unsigned long *rs_ptr) {{


	const uint8_t key[{idx}][16] = {{{{0x10, 0xa5, 0x88, 0x69, 0xd7, 0x4b, 0xe5, 0xa3, 0x74, 0xcf, 0x86, 0x7c, 0xfb, 0x47, 0x38, 0x59}},
"""

f.write(next_string.format(idx = str(repetitions)))

next_string = "							    {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf1, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01},\n"

for i in range(repetitions - 2):
    f.write(next_string)

next_string = """								{0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf1, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01}};

	uint8_t plaintext[MAX_PARALLEL_AES][16];
	uint8_t RoundKey[MAX_PARALLEL_AES][176];

"""

f.write(next_string)

next_string = "	#pragma HLS array_partition variable = sbox{idx}\n"

for i in range(repetitions):
    f.write(next_string.format(idx = str(i)))

next_string = """
    #pragma HLS array_partition variable = key dim = 1
	#pragma HLS array_partition variable = plaintext dim = 1
	#pragma HLS array_partition variable = RoundKey dim = 1
    
	for (int i = 0; i < MAX_PARALLEL_AES; i++) {
		#pragma HLS unroll
		((long *)(plaintext[i]))[0] = 0;
		((long *)(plaintext[i]))[1] = pt_ptr[i];
	}

"""

f.write(next_string)

next_string = "	KeyExpansion(RoundKey[{idx}], key[{idx}]);\n"

for i in range(repetitions):
    f.write(next_string.format(idx = str(i)))

f.write("\n")

next_string = "	Encrypt_AES{idx}((state_t*)(plaintext[{idx}]), RoundKey[{idx}]);\n"

for i in range(repetitions):
    f.write(next_string.format(idx = str(i)))

next_string = """
	// Writeback
	for (int i = 0; i < MAX_PARALLEL_AES; i++) {
		#pragma HLS unroll
		rs_ptr[2*i] = ((long *)(plaintext[i]))[0];
		rs_ptr[2*i + 1] = ((long *)(plaintext[i]))[1];
	}
}

#ifdef __VITIS_CL__ // for lab 3SubBytes
} // extern
#endif

void KeyExpansion(uint8_t* RoundKey, const uint8_t* Key) {
    uint8_t i, j, k;
    uint8_t tempa[4];

    // The first round key is the key itself.
    for (i = 0; i < 4; i++){
		#pragma HLS unroll
        RoundKey[(i * 4) + 0] = Key[(i * 4) + 0];
        RoundKey[(i * 4) + 1] = Key[(i * 4) + 1];
        RoundKey[(i * 4) + 2] = Key[(i * 4) + 2];
        RoundKey[(i * 4) + 3] = Key[(i * 4) + 3];
    }

    // All other round keys are found from the previous round keys.
    for (i = 4; i < 4 * (10 + 1); i++) {
        k = (i - 1) * 4;
        tempa[0]=RoundKey[k + 0];
        tempa[1]=RoundKey[k + 1];
        tempa[2]=RoundKey[k + 2];
        tempa[3]=RoundKey[k + 3];

        if (i % 4 == 0) {
            // Shifts the 4 bytes in a word to the left once.
            // [a0,a1,a2,a3] becomes [a1,a2,a3,a0]
            const uint8_t u8tmp = tempa[0];
            tempa[0] = tempa[1];
            tempa[1] = tempa[2];
            tempa[2] = tempa[3];
            tempa[3] = u8tmp;

            // Takes a four-byte input word and  applies the S-box to each of the four bytes to produce an output word.
            tempa[0] = getSBoxValue0(tempa[0]);
            tempa[1] = getSBoxValue0(tempa[1]);
            tempa[2] = getSBoxValue0(tempa[2]);
            tempa[3] = getSBoxValue0(tempa[3]);

            tempa[0] = tempa[0] ^ Rcon[i / 4];
        }

        j = i * 4; k = (i - 4) * 4;
        RoundKey[j + 0] = RoundKey[k + 0] ^ tempa[0];
        RoundKey[j + 1] = RoundKey[k + 1] ^ tempa[1];
        RoundKey[j + 2] = RoundKey[k + 2] ^ tempa[2];
        RoundKey[j + 3] = RoundKey[k + 3] ^ tempa[3];
    }
}
"""

f.write(next_string)

next_string = """
// COPY {idx}

uint8_t getSBoxValue{idx}(uint8_t n) {{
    return sbox{idx}[n];
}}
// Main function to encrypt the plain text with the AES encryption scheme.
void Encrypt_AES{idx}(state_t* state, const uint8_t* RoundKey) {{

	// Add the first round key to the state before starting the rounds.
	AddRoundKey{idx}(0, state, RoundKey);

    for (uint8_t round = 1; round <= 10; round++) {{
    	SubBytes{idx}(state);
        ShiftRows{idx}(state);
        // In the final round the MixColumn operation is ommitted.
        if (round == 10) {{
            break;
        }}
        MixColumns{idx}(state);
        AddRoundKey{idx}(round, state, RoundKey);
    }}
    // Add round key to last round
    AddRoundKey{idx}(10, state, RoundKey);
}}

void AddRoundKey{idx}(uint8_t round, state_t* state, const uint8_t* RoundKey) {{
    for (uint8_t i = 0; i < 4; ++i) {{
		#pragma HLS unroll
        for (uint8_t j = 0; j < 4; ++j) {{
			#pragma HLS unroll
            (*state)[i][j] ^= RoundKey[(round * 4 * 4) + (i * 4) + j];
        }}
    }}
}}

// Substitutes the values in the state matrix with values in an S-box.
void SubBytes{idx}(state_t* state) {{
    for (uint8_t i = 0; i < 4; i++) {{
		#pragma HLS unroll
        for (uint8_t j = 0; j < 4; j++) {{
			#pragma HLS unroll
            (*state)[j][i] = getSBoxValue{idx}((*state)[j][i]);
        }}
    }}
}}

void ShiftRows{idx}(state_t* state) {{
    uint8_t temp;

    // Rotate Row 1 to the left 1 time
    temp = (*state)[0][1];
    (*state)[0][1] = (*state)[1][1];
    (*state)[1][1] = (*state)[2][1];
    (*state)[2][1] = (*state)[3][1];
    (*state)[3][1] = temp;

    // Rotate Row 2 to the left 2 times
    temp = (*state)[0][2];
    (*state)[0][2] = (*state)[2][2];
    (*state)[2][2] = temp;

    temp = (*state)[1][2];
    (*state)[1][2] = (*state)[3][2];
    (*state)[3][2] = temp;

    // Rotate Row 3 to the left 3 times
    temp = (*state)[0][3];
    (*state)[0][3] = (*state)[3][3];
    (*state)[3][3] = (*state)[2][3];
    (*state)[2][3] = (*state)[1][3];
    (*state)[1][3] = temp;
}}

uint8_t xtime{idx}(uint8_t x) {{
    return ((x<<1) ^ (((x>>7) & 1) * 0x1b));
}}

void MixColumns{idx}(state_t* state) {{
    uint8_t Tmp, Tm, t;
    for (uint8_t i = 0; i < 4; ++i) {{
		#pragma HLS unroll
        t   = (*state)[i][0];
        Tmp = (*state)[i][0] ^ (*state)[i][1] ^ (*state)[i][2] ^ (*state)[i][3];
        Tm  = (*state)[i][0] ^ (*state)[i][1]; Tm = xtime{idx}(Tm); (*state)[i][0] ^= Tm ^ Tmp;
        Tm  = (*state)[i][1] ^ (*state)[i][2]; Tm = xtime{idx}(Tm); (*state)[i][1] ^= Tm ^ Tmp;
        Tm  = (*state)[i][2] ^ (*state)[i][3]; Tm = xtime{idx}(Tm); (*state)[i][2] ^= Tm ^ Tmp;
        Tm  = (*state)[i][3] ^ t; Tm = xtime{idx}(Tm); (*state)[i][3] ^= Tm ^ Tmp;
    }}
}}

"""

for i in range(repetitions):
    f.write(next_string.format(idx = str(i)))

f.close()