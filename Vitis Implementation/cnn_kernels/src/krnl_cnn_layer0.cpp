/****************************************************************
 * Copyright (c) 2020~2022, 18-643 Course Staff, CMU
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:

 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.

 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANYg0_power_modulo: for (int j = 0; j < 64; j++) {
    		if (plaintext[i] % 2 == 1) {
    			g_accum[i] = (g_accum[i] * g_bases[j]) % n2;
    		}
    		plaintext[i] = plaintext[i] >> 1;
    	} DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.

 * The views and conclusions contained in the software and
 * documentation are those of the authors and should not be
 * interpreted as representing official policies, either expressed or
 * implied, of the FreeBSD Project.
 ****************************************************************/

/*
 * CMU 18643 Fall 2022 Lab Exercise
 *
 * You can edit this file
 */

/****************************************************************
 * Blocked convolution layer implementation
 * based on Figure 5:
 *    C. Zhang, et al., "Optimizing FPGA-based Accelerator
 *    Design for Deep Convolutional Neural Networks," FPGA, 2015.
 ****************************************************************/

#include "krnl_cnn_layer0.h"

// Prevent aliasing
#undef BATCH_SIZE

#include "util643.h"

#ifdef __VITIS_CL__
extern "C" {
#endif
// Paillier Implementation

void krnl_cnn_layer0(unsigned long *pt_ptr,
		             unsigned long *rs_ptr) {

	unsigned long plaintext[MAX_PARALLEL_PAILLIER];
	unsigned long ciphertext[MAX_PARALLEL_PAILLIER];
	unsigned long r_accum;
	unsigned long g_accum[MAX_PARALLEL_PAILLIER];

    const unsigned long g_bases[64] = {35, 1225, 1500625, 162429067247, 288373890606, 615521409983, 750134396060, 808268167391,
                                       322844556745, 821793923936, 557458688246, 569244509925, 3349788884, 341814719242, 679003006251, 774853645491,
                                       533777373487, 95561915164, 1041052720952, 671801338531, 633902390976, 251181430739, 810245275953, 534082928677,
                                       522230337518, 207041518478, 740758678285, 130025092914, 699653031586, 270961292594, 54757714147, 166012124223,
                                       864832445681, 81976759049, 324391558715, 624493519045, 147159188200, 381007713848, 450686147511, 633385009274,
                                       178710704016, 434548354710, 1010625034967, 196750272512, 295356557483, 116188785267, 214129614340, 333516153976,
                                       178207231701, 427814483106, 718885740114, 453109136417, 358963896204, 793462810642, 242309609092, 1038667183435,
                                       765265065497, 977829025796, 694673911703, 783956922650, 600030782113, 523132375126, 97212375444, 943385767414};

	#pragma HLS array_partition variable = ciphertext
	#pragma HLS array_partition variable = plaintext
	#pragma HLS array_partition variable = g_accum
	#pragma HLS array_partition variable = g_bases

    // Constants required for Paillier Cryptosystem
    const unsigned long p = 1009;
    const unsigned long q = 1013;
    const unsigned long n = 1009 * 1013; //1022117
    const unsigned long n2 = n * n;
    const unsigned long r = 7;
    const unsigned long g = 35;

    r_accum = (unsigned long)492418136798;

    for (int i = 0; i < MAX_PARALLEL_PAILLIER; i++) {
		#pragma HLS unroll
    	plaintext[i] = pt_ptr[i];
    	g_accum[i] = 1;
    }

    for (int i = 0; i < MAX_PARALLEL_PAILLIER; i++) {
		#pragma HLS unroll

    	g0_power_modulo: for (int j = 0; j < 64; j++) {
    		if (plaintext[i] % 2 == 1) {
    			g_accum[i] = (g_accum[i] * g_bases[j]) % n2;
    		}
    		plaintext[i] = plaintext[i] >> 1;
    	}
    	ciphertext[i] = (r_accum * g_accum[i]) % n2;
	}

    for (int i = 0; i < MAX_PARALLEL_PAILLIER; i++) {
		#pragma HLS unroll
        rs_ptr[i] = ciphertext[i];
    }
}

#ifdef __VITIS_CL__ // for lab 3
} // extern
#endif
