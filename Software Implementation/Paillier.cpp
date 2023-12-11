
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include "Paillier.h"

// Paillier Cryptosystem - https://en.wikipedia.org/wiki/Paillier_cryptosystem
// m - Plaintext, message to be encrypted
// c - Ciphertext, encrypted message
// n - pq, Product of two large prime numbers such that gcd(pq, (p-1)(q-1)) = 1
// r - Random number such that 0 < r < n and gcd(r,n) = 1
// g - Random number where g such that 0 <= g < n2-1 and gcd(g, n2) = 1

void PaillierEncrypt(unsigned long plaintext) {
    // Constants required for Paillier Cryptosystem
    unsigned long p, q, n, n2, r, g;
    p = 1009;
    q = 1013;
    n = 1009 * 1013; //1022117
    n2 = n * n;
    r = 7; 
    g = 35;

    const unsigned long g_bases[64] = {35, 1225, 1500625, 162429067247, 288373890606, 615521409983, 750134396060, 808268167391,
                                       322844556745, 821793923936, 557458688246, 569244509925, 3349788884, 341814719242, 679003006251, 774853645491,
                                       533777373487, 95561915164, 1041052720952, 671801338531, 633902390976, 251181430739, 810245275953, 534082928677,
                                       522230337518, 207041518478, 740758678285, 130025092914, 699653031586, 270961292594, 54757714147, 166012124223,
                                       864832445681, 81976759049, 324391558715, 624493519045, 147159188200, 381007713848, 450686147511, 633385009274,
                                       178710704016, 434548354710, 1010625034967, 196750272512, 295356557483, 116188785267, 214129614340, 333516153976,
                                       178207231701, 427814483106, 718885740114, 453109136417, 358963896204, 793462810642, 242309609092, 1038667183435,
                                       765265065497, 977829025796, 694673911703, 783956922650, 600030782113, 523132375126, 97212375444, 943385767414};

    unsigned long ciphertext, r_accum, g_accum;

    r_accum = (unsigned long)492418136798;
    g_accum = 1;

    for (int j = 0; j < 64; j++) {

        if (plaintext % 2 == 1) {
    		g_accum = (g_accum * g_bases[j]) % n2;
    	}

    	plaintext = plaintext >> 1;
    }

    ciphertext = r_accum * g_accum;
    ciphertext = ciphertext % n2;

    // printf("Data: %lu, Result: %lu\n", plaintext, ciphertext);
}
