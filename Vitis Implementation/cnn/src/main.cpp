/****************************************************************
 * Copyright (c) 2017~2022, 18-643 Course Staff, CMU
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
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <deque>
#include "lab3_kernels.h"
#include "cnn_helper.h"
#include <assert.h>
#include <sys/time.h>

// Request Structs
typedef struct {
    unsigned long data;
    struct timeval rxTime;
} enc_req_t;

typedef std::deque<enc_req_t> enc_queue_t;

// Function Headers
void HandleAESEncrypt(enc_queue_t &buffer);
void HandlePaillierEncrypt(enc_queue_t &buffer);
inline void SwitchAESKernel();
inline void SwitchPaillierKernel();

// Globals
unsigned long encryptCount; // Count of Encryptions that has happened so far
unsigned long LatencyUS;
unsigned long PRCount;
unsigned long PaillierCount;
unsigned long AESCount;
int active_encryption;
struct timeval startTime;

// Global Kernel Struct
cl_object cl_obj;
krnl_object cnn_obj[3];

// Global Input / Output Pointer Arguments
unsigned long *ptr_AES_input;
unsigned long *ptr_AES_output;
unsigned long *ptr_Paillier_input;
unsigned long *ptr_Paillier_output;

int main(int argc, char* argv[]) {

	// Time struct
    struct timeval endTime;

    // Open and read data file
    FILE *dataFP = fopen("/mnt/sd-mmcblk0p1/data.txt", "r");
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

    // Initializations
    encryptCount = 0;
    LatencyUS = 0;
    PRCount = 0;
    PaillierCount = 0;
    AESCount = 0;

#ifdef __VITIS_CL__
    // Hard coding xclbin filenames, ignoring command line arguments
    std::string xclbinFilename[3] = {
        "binary_container_X.xclbin",
        "binary_container_0.xclbin", // Paillier
        "binary_container_1.xclbin"  // AES
    };
#endif

    cnn_obj[0].index = 0;
    cnn_obj[0].name = "krnl_cnn_layer0";

    cnn_obj[1].index = 1;
    cnn_obj[1].name = "krnl_cnn_layer1";
//
//    cnn_obj[2].index = 2;
//    cnn_obj[2].name = "krnl_cnn_layerX";

#ifdef __VITIS_CL__
    std::cout << "===== Initialize device ======" << std::endl;
    initialize_device(cl_obj);

    std::cout << "===== Reading xclbin ======" << std::endl;
    // Read cnn
    read_xclbin(xclbinFilename[1], cl_obj.bins);
    read_xclbin(xclbinFilename[2], cl_obj.bins);
//    read_xclbin(xclbinFilename[0], cl_obj.bins);
#endif

    // Start with Paillier Kernel by Default
    SwitchPaillierKernel();

    // Allocate Buffers
    std::cout << "\n===== Allocating buffers ======" << std::endl;
    uint64_t bufIdx=0;

    // Paillier Input Buffer
    allocate_readonly_mem(cl_obj, (void**) &ptr_Paillier_input, bufIdx++, MAX_PARALLEL_PAILLIER * sizeof(unsigned long));
    // Paillier Output Buffer
    allocate_readwrite_mem(cl_obj, (void**) &ptr_Paillier_output, bufIdx++, MAX_PARALLEL_PAILLIER * sizeof(unsigned long));
    // AES Input Buffer
    allocate_readonly_mem(cl_obj, (void**) &ptr_AES_input, bufIdx++, MAX_PARALLEL_AES * sizeof(unsigned long));
    // AES Output Buffer
    allocate_readwrite_mem(cl_obj, (void**) &ptr_AES_output, bufIdx++, 2 * MAX_PARALLEL_AES * sizeof(unsigned long));

    // Report Project
    printf("Max Parallel Paillier: %d\n", MAX_PARALLEL_PAILLIER);
    printf("Max Parallel AES: %d\n", MAX_PARALLEL_AES);
    printf("Buffer Size: %d\n", BUFFER_SIZE);

    std::cout << "\n===== Execution and Timing starts ======" << std::endl;
    gettimeofday(&startTime, NULL);

    while ((readSize = getline(&line, &len, dataFP)) != -1) {
        stringIndex = strcspn(line,",");
        if ((stringIndex == 3) && (strncmp(line, "AES", stringIndex) == 0)) {
            // AES Encryption
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
            // Paillier Encryption
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

        if ((requestCount % 40000) == 0) printf("Request Count: %lu\n", requestCount);
    }

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


    // Populate Buffers for Testing
//    for (unsigned long i = 0; i < 2*MAX_PARALLEL_PAILLIER; i++) {
//    	enc_req_t request;
//    	gettimeofday(&(request.rxTime), NULL);
//    	request.data = i;
//    	buffer[0].push_back(request);
//    }
//
//    for (int i = 0; i < 2*MAX_PARALLEL_AES; i++) {
//    	enc_req_t request;
//        gettimeofday(&(request.rxTime), NULL);
//        request.data = i;
//        buffer[1].push_back(request);
//    }
//
//    // Temporary Test Vectors
//    HandlePaillierEncrypt(buffer[0]);
//    SwitchAESKernel();
//    HandleAESEncrypt(buffer[1]);
//    SwitchPaillierKernel();
//    HandlePaillierEncrypt(buffer[0]);
//    SwitchAESKernel();
//    HandleAESEncrypt(buffer[1]);

//#ifdef __VITIS_CL__
//#ifdef ENABLE_DFX
//    cnn_run_kernel(cl_obj, cnn_obj[0], cnn_obj[1]);
//#else
//    cnn_run_kernel(cl_obj, cnn_obj, cnn_obj);
//#endif
//#else
//#ifdef ENABLE_DFX
//    krnl_cnn_layer0(ptr_Paillier_input, ptr_Paillier_output, MAX_PARALLEL_PAILLIER);
//    krnl_cnn_layer1(ptr_AES_input, ptr_AES_output, MAX_PARALLEL_AES);
//#endif
////#else
////    krnl_cnn_layerX(ptr_input, ptr_weight[0], ptr_output[0], BATCH_SIZE, R_OFM(0), C_OFM(0), M_OFM(0), N_IFM(0));
////    krnl_cnn_layerX(ptr_output[0], ptr_weight[1], ptr_output[1], BATCH_SIZE, R_OFM(1), C_OFM(1), M_OFM(1), N_IFM(1));
////#endif
//#endif

//#ifdef __VITIS_CL__
//#ifdef ENABLE_DFX
//    cnn_run_kernel(cl_obj, cnn_obj[0], cnn_obj[1]);
//#else
//    cnn_run_kernel(cl_obj, cnn_obj, cnn_obj);
//#endif
//#else
//#ifdef ENABLE_DFX
//    krnl_cnn_layer0(ptr_input, ptr_weight[0], ptr_output[0], BATCH_SIZE);
//    krnl_cnn_layer1(ptr_output[0], ptr_weight[1], ptr_output[1], BATCH_SIZE);
//#else
//    krnl_cnn_layerX(ptr_input, ptr_weight[0], ptr_output[0], BATCH_SIZE, R_OFM(0), C_OFM(0), M_OFM(0), N_IFM(0));
//    krnl_cnn_layerX(ptr_output[0], ptr_weight[1], ptr_output[1], BATCH_SIZE, R_OFM(1), C_OFM(1), M_OFM(1), N_IFM(1));
//#endif
//#endif

    gettimeofday(&endTime, NULL);
    std::cout << "Execution and Timing finished!\n" << std::endl;

    // Result Management
    printf("AES Buffer Size: %zu\n", buffer[1].size());
    printf("Paillier Buffer Size: %zu\n", buffer[0].size());
    printf("Last Active Kernel: %d\n", active_encryption);

    // Calculate runtime and latency
    float timeusec = (endTime.tv_sec - startTime.tv_sec)*1e6 + (endTime.tv_usec - startTime.tv_usec);
    unsigned long averageLatencyusec = LatencyUS / encryptCount;

    printf("Number of Encryptions: %lu\n", encryptCount);
    printf("Runtime = %0.1f (microsec)\n", timeusec);
    printf("Average Latency = %lu (microsec)\n", averageLatencyusec);
    printf("Number of PR: %lu\n", PRCount);
    printf("Number of Paillier Kernels Run: %lu\n", PaillierCount);
    printf("Number of AES Kernels Run: %lu\n", AESCount);

    // Manage Memory
    deallocate_mem(cl_obj, ptr_Paillier_input, 0);
    deallocate_mem(cl_obj, ptr_Paillier_output, 1);
    deallocate_mem(cl_obj, ptr_AES_input, 2);
    deallocate_mem(cl_obj, ptr_AES_output, 3);

    // File pointer management
    fclose(dataFP);
    if (line) free(line);

    std::cout << "\n===== Exiting ======" << std::endl;

    return 0;
}

inline void SwitchAESKernel() {
	// std::cout << "\n===== Programming AES Kernel ======" << std::endl;

	// Update Statistics
	active_encryption = 1;
	PRCount += 1;

	// Program Kernel
#ifdef __VITIS_CL__
	program_kernel(cl_obj, cnn_obj[1]);
#endif
}

inline void SwitchPaillierKernel() {
	// std::cout << "\n===== Programming Paillier Kernel ======" << std::endl;

	// Update Statistics
	active_encryption = 0;
	PRCount += 1;

	// Program Kernel
#ifdef __VITIS_CL__
	program_kernel(cl_obj, cnn_obj[0]); // Paillier Kernel
#endif
}

void HandlePaillierEncrypt(enc_queue_t &buffer) {

    // Assumption is that kernel is already programmed
	assert(active_encryption == 0);

	// Request Time Struct
	struct timeval requestTime[MAX_PARALLEL_PAILLIER];
	struct timeval encryptTime;

	int validReq = 0;

	// Copy Data and Store Request Time
	for (int i = 0; i < MAX_PARALLEL_PAILLIER; i++) {
		if (buffer.size() == 0) {
			ptr_Paillier_input[i] = 0;
		}
		else {
			ptr_Paillier_input[i] = buffer.front().data;
			requestTime[i] = buffer.front().rxTime;
			buffer.pop_front();
			encryptCount++;
			validReq++;
		}
	}

	// Process Data //
	// std::cout << "Running Paillier kernel" << std::endl;

#ifdef __VITIS_CL__

	// Run Kernel
	cl_int err;

	// Get i/o buffers from kernel object
	cl::Buffer *buffer_in = &cl_obj.buffers[0];
	cl::Buffer *buffer_out = &cl_obj.buffers[1];

	// Set the kernel Arguments
	uint64_t narg = 0;
	OCL_CHECK(err, err = cl_obj.krnl->setArg(narg++, *buffer_in));  // Inputs
	OCL_CHECK(err, err = cl_obj.krnl->setArg(narg++, *buffer_out)); // Outputs

	// Data will be migrated to kernel space
	OCL_CHECK(err, err = cl_obj.q.enqueueMigrateMemObjects({*buffer_in}, 0/* 0 means from host*/));

	// Launch the Kernel; this is nonblocking.
	OCL_CHECK(err, err = cl_obj.q.enqueueTask(*cl_obj.krnl));

	// The result of the previous kernel execution will need to be retrieved in
	// order to view the results. This call will transfer the data from FPGA to
	// source_results vector
	OCL_CHECK(err, cl_obj.q.enqueueMigrateMemObjects({*buffer_out}, CL_MIGRATE_MEM_OBJECT_HOST));

	// Wait for all tasks to finish
	OCL_CHECK(err, cl_obj.q.finish());

#else
	// Run Software
	krnl_cnn_layer0(ptr_Paillier_input, ptr_Paillier_output);
#endif

	// std::cout << "Paillier Kernel Execution completed" << std::endl;

	// Update Latency
	gettimeofday(&encryptTime, NULL);

	PaillierCount++;

	for (int i = 0; i < MAX_PARALLEL_PAILLIER; i++) {
		if (i < validReq) {
			// This is a valid request
			// Update latency
			unsigned long latency = (encryptTime.tv_sec - requestTime[i].tv_sec) * 1000000 +
			                        (encryptTime.tv_usec - requestTime[i].tv_usec);
			LatencyUS += latency;
			// printf("Paillier %d, Result: %lu\n", i, ptr_Paillier_output[i]);
		}
	}
}

void HandleAESEncrypt(enc_queue_t &buffer) {

    // Assumption is that kernel is already programmed
	assert(active_encryption == 1);

	// Request Time Struct
	struct timeval requestTime[MAX_PARALLEL_AES];
	struct timeval encryptTime;

	int validReq = 0;

	// Copy Data and Store Request Time
	for (int i = 0; i < MAX_PARALLEL_AES; i++) {
		if (buffer.size() == 0) {
			ptr_AES_input[i] = 0;
		}
		else {
			ptr_AES_input[i] = buffer.front().data;
			requestTime[i] = buffer.front().rxTime;
			buffer.pop_front();
			encryptCount++;
			validReq++;
		}
	}

	// Process Data //
	// std::cout << "Running AES kernel" << std::endl;

#ifdef __VITIS_CL__

	// Run Kernel
	cl_int err;

	// Get i/o buffers from kernel object
	cl::Buffer *buffer_in = &cl_obj.buffers[2];
	cl::Buffer *buffer_out = &cl_obj.buffers[3];

	// Set the kernel Arguments
	uint64_t narg = 0;
	OCL_CHECK(err, err = cl_obj.krnl->setArg(narg++, *buffer_in));  // Inputs
	OCL_CHECK(err, err = cl_obj.krnl->setArg(narg++, *buffer_out)); // Outputs

	// Data will be migrated to kernel space
	OCL_CHECK(err, err = cl_obj.q.enqueueMigrateMemObjects({*buffer_in}, 0/* 0 means from host*/));

	// Launch the Kernel; this is nonblocking.
	OCL_CHECK(err, err = cl_obj.q.enqueueTask(*cl_obj.krnl));

	// The result of the previous kernel execution will need to be retrieved in
	// order to view the results. This call will transfer the data from FPGA to
	// source_results vector
	OCL_CHECK(err, cl_obj.q.enqueueMigrateMemObjects({*buffer_out}, CL_MIGRATE_MEM_OBJECT_HOST));

	// Wait for all tasks to finish
	OCL_CHECK(err, cl_obj.q.finish());

#else
	// Run Software
	krnl_cnn_layer1(ptr_AES_input, ptr_AES_output);
#endif

	// std::cout << "AES Kernel Execution completed" << std::endl;

	// Update Latency
	gettimeofday(&encryptTime, NULL);

	AESCount++;

	for (int i = 0; i < MAX_PARALLEL_AES; i++) {
		if (i < validReq) {
			// This is a valid request
			// Update latency
			unsigned long latency = (encryptTime.tv_sec - requestTime[i].tv_sec) * 1000000 +
			                        (encryptTime.tv_usec - requestTime[i].tv_usec);
			LatencyUS += latency;
			// printf("AES %d, Result: %lu %lu\n", i, ptr_AES_output[2*i], ptr_AES_output[2*i + 1]);
		}
	}
}


