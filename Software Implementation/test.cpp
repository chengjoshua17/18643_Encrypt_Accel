#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <deque>
#include <sys/time.h>
#include "AES.h"
#include "Paillier.h"

#define MAX_PARALLEL_AES 2
#define MAX_PARALLEL_PAILLIER 7
#define BUFFER_SIZE 1000

typedef struct {
    unsigned long data;
    struct timeval rxTime;
} enc_req_t;

typedef std::deque<enc_req_t> enc_queue_t;

void HandleAESEncrypt(enc_queue_t &buffer);
void HandlePaillierEncrypt(enc_queue_t &buffer);
void SwitchAESKernel();
void SwitchPaillierKernel();

// Globals
unsigned long encryptCount; // Count of Encryptions that has happened so far
unsigned long LatencyUS;
unsigned long PRCount;
int active_encryption;
struct timeval startTime;

int main() {
    
    // Open and read data file
    FILE *dataFP = fopen("data.txt", "r");
    if (dataFP == NULL) {
        printf("Error: File not found!");
        return -1;
    }

    // Variables for Parsing String
    ssize_t readSize;
    char *line = NULL;
    size_t len = 0;
    size_t stringIndex;
    unsigned long data;
    unsigned long requestCount = 0;

    // Buffer for Storing and Queuing Up Encryptions
    // buffer[0] is Paillier
    // buffer[1] is AES
    enc_queue_t buffer[2];

    // By default. we start at Paillier
    active_encryption = 0;

    struct timeval endTime;

    // Initializations
    encryptCount = 0;
    LatencyUS = 0;
    PRCount = 0;
    
    // Get startTime
    gettimeofday(&startTime, NULL);

    while ((readSize = getline(&line, &len, dataFP)) != -1) {
        // printf("%s", line);
        stringIndex = strcspn(line,",");
        if ((stringIndex == 3) && (strncmp(line, "AES", stringIndex) == 0)) {
            // printf("This is AES Encryption!\n");
            data = strtoul(line + 4, NULL, 10);

            // Parse and Store Request
            enc_req_t request;
            gettimeofday(&(request.rxTime), NULL);
            request.data = data;
            buffer[1].push_back(request);

            requestCount++;
            // printf("AES Buffer Size: %zu\n", buffer[1].size());

            if(active_encryption == 1) {
                // AES is active
                if (buffer[1].size() >= MAX_PARALLEL_AES) HandleAESEncrypt(buffer[1]);
            }
            else {
                // Paillier is active
                if (buffer[1].size() >= BUFFER_SIZE) {
                    // Set AES Active
                    SwitchAESKernel();

                    while (buffer[1].size() >= MAX_PARALLEL_AES) {
                        HandleAESEncrypt(buffer[1]);
                    }
                }
            }
        }
        else if ((stringIndex == 8) && (strncmp(line, "Paillier", stringIndex) == 0)) {
            // printf("This is Paillier Encryption!\n");
            data = strtoul(line + 9, NULL, 10);

            // Parse and Store Request
            enc_req_t request;
            gettimeofday(&(request.rxTime), NULL);
            request.data = data;
            buffer[0].push_back(request);

            requestCount++;

            if(active_encryption == 0) {
                // Paillier is active
                if (buffer[0].size() >= MAX_PARALLEL_PAILLIER) HandlePaillierEncrypt(buffer[0]);
            }
            else {
                // AES is active
                if (buffer[0].size() >= BUFFER_SIZE) {
                    // Set Paillier Active
                    SwitchPaillierKernel();

                    while (buffer[0].size() >= MAX_PARALLEL_PAILLIER) {
                        HandlePaillierEncrypt(buffer[0]);
                    }
                }
            }
        }
        else {
            printf("Unrecognized Encryption!");
            return -1;
        }
    }
  
    // printf("AES Buffer Size: %zu\n", buffer[1].size());
    // printf("Paillier Buffer Size: %zu\n", buffer[0].size());
    // printf("Last Active Kernel: %d\n", active_encryption);

    // Clean up remaining queued up encryption
    if (active_encryption == 1) {
        while (buffer[1].size() != 0) {
            HandleAESEncrypt(buffer[1]);
        }
        if (buffer[0].size() != 0) SwitchPaillierKernel();

        while (buffer[0].size() != 0) {
            HandlePaillierEncrypt(buffer[0]);
        }
    }
    else {
        while (buffer[0].size() != 0) {
            HandlePaillierEncrypt(buffer[0]);
        }

        if (buffer[1].size() != 0) SwitchAESKernel();

        while (buffer[1].size() != 0) {
            HandleAESEncrypt(buffer[1]);
        }
    }

    printf("AES Buffer Size: %zu\n", buffer[1].size());
    printf("Paillier Buffer Size: %zu\n", buffer[0].size());
    printf("Last Active Kernel: %d\n", active_encryption);

    gettimeofday(&endTime, NULL);

    // Calculate Runtime
    float timeusec = (endTime.tv_sec - startTime.tv_sec)*1e6 + (endTime.tv_usec - startTime.tv_usec);

    // Calculate Latency
    unsigned long averageLatencyusec = LatencyUS / requestCount;

    // Report
    printf("Number of Encryptions: %lu\n", requestCount);
    printf("Runtime = %0.1f (microsec)\n", timeusec);
    printf("Average Latency = %lu (microsec)\n", averageLatencyusec);
    printf("Number of PR: %lu\n", PRCount);

    fclose(dataFP);
    if (line) free(line);
    
    return 0;
}

void SwitchAESKernel() {
    // printf("Switching to AES Kernel!\n");
    active_encryption = 1;
    PRCount += 1;
    return;
}

void SwitchPaillierKernel() {
    // printf("Switching to Paillier Kernel!\n");
    active_encryption = 0;
    PRCount += 1;
    return;
}

void HandleAESEncrypt(enc_queue_t &buffer) {

    // Get encrypt Time
    struct timeval encryptTime;
    gettimeofday(&encryptTime, NULL);
    
    for (int i = 0; i < MAX_PARALLEL_AES; i++) {
        if (buffer.size() == 0) return;
        AESEncrypt(buffer.front().data);

        // Update latency
        unsigned long latency = (encryptTime.tv_sec - buffer.front().rxTime.tv_sec) * 1000000 +
                                (encryptTime.tv_usec - buffer.front().rxTime.tv_usec);
        LatencyUS += latency;

        buffer.pop_front();
        encryptCount++;
    }
}

void HandlePaillierEncrypt(enc_queue_t &buffer) {
    
    // Get encrypt Time
    struct timeval encryptTime;
    gettimeofday(&encryptTime, NULL);
    
    for (int i = 0; i < MAX_PARALLEL_PAILLIER; i++) {
        if (buffer.size() == 0) return;
        PaillierEncrypt(buffer.front().data);

        // Update latency
        unsigned long latency = (encryptTime.tv_sec - buffer.front().rxTime.tv_sec) * 1000000 +
                                (encryptTime.tv_usec - buffer.front().rxTime.tv_usec);
        LatencyUS += latency;

        buffer.pop_front();
        encryptCount++;
    }
}
