//
//  PanHandle
//
//  Created by John Crepezzi on 11/25/13.
//  Copyright (c) 2013 John Crepezzi. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OpenCL/opencl.h>

#include "base64.h"
#include "luhn_check.cl.h"
#include "msha_kernel.cl.h"

#define NUM_VALUES 1024
#define KEY_LENGTH 16

// Our main function
int main(int argc, const char * argv[]) {

    // read in all lines
    char lines[1022][28];
    FILE *fp = fopen("/Users/john/Development/PanHandle/data/hashes.txt", "r");
    for (int i = 0; i < 1022; i++) { // TODO flex
        fgets(lines[i], 28, fp);
    }
    fclose(fp);
    
    // decode each line
    char choices[1022][20];
    for (int i = 0; i < 1022; i++) { // TODO flex
        Base64decode(choices[i], lines[i]);
    }
        
    // Create out dispatch queue, prefer GPU but allow fallback
    dispatch_queue_t queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_GPU, NULL);
    if (queue == NULL) {
        queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_CPU, NULL);
    }
    
    // Put together our test data
    cl_ulong* candidates = (cl_ulong*)malloc(sizeof(cl_ulong) * NUM_VALUES);
    for (int i = 0; i < NUM_VALUES; i++) {
        candidates[i] = 5462916022532590L + i;
    }
    printf("starting with %d\n", NUM_VALUES);
    
    // perform our luhn checks, setting anything to 0 that doesn't pass
    void* cl_candidates = gcl_malloc(sizeof(cl_ulong) * NUM_VALUES, candidates, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
    dispatch_sync(queue, ^{
        size_t wgs;
        gcl_get_kernel_block_workgroup_info(luhn_check_kernel, CL_KERNEL_WORK_GROUP_SIZE, sizeof(wgs), &wgs, NULL);
        cl_ndrange range = { 1, {0, 0, 0}, {NUM_VALUES, 0, 0}, {wgs, 0, 0}};
        luhn_check_kernel(&range, (cl_ulong*)cl_candidates);
        gcl_memcpy(candidates, cl_candidates, sizeof(cl_ulong) * NUM_VALUES);
    });
    
    // count the number of remaining numbers
    unsigned int count = 0;
    for (int j = 0; j < NUM_VALUES; j++) {
        if (candidates[j] != 0) {
            count += 1;
        }
    }
    printf("left with %d after luhn\n", count);
    
    // and convert the leftovers into a char array
    cl_uchar numbers[count * 16 * sizeof(cl_uchar)];
    void *ptr = numbers;
    for (int j = 0; j < NUM_VALUES; j++) {
        if (candidates[j] != 0) {
            sprintf(ptr, "%lld", candidates[j]);
            ptr += sizeof(unsigned char) * 16;
        }
    }
    
    // now we're ready to SHA1 the keys
    void* cl_keys = gcl_malloc(sizeof(cl_uchar) * count * 16, numbers, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_results = gcl_malloc(sizeof(cl_uint) * count * 5, NULL, CL_MEM_WRITE_ONLY);
    void* hashes = malloc(sizeof(cl_uint) * count * 5);
    dispatch_sync(queue, ^{
        size_t wgs;
        gcl_get_kernel_block_workgroup_info(sha1_crypt_kernel_kernel, CL_KERNEL_WORK_GROUP_SIZE, sizeof(wgs), &wgs, NULL);
        cl_ndrange range = { 1, {0, 0, 0}, {count, 0, 0}, {wgs, 0, 0}};
        sha1_crypt_kernel_kernel(&range, (cl_uint*)cl_keys, (cl_uint*)cl_results);
        gcl_memcpy(hashes, cl_results, sizeof(cl_uint) * count * 5);
    });
    printf("produced SHA");
    
    // we've got outselves the keys, and now we can go through them looking for matches
    // TODO use hashmap
    for (int i = 0; i < 1022; i++) {
        void *ptr = hashes;
        for (int j = 0; j < count; j++) {
            if (memcmp(choices[i], ptr, 5) == 0) {
                printf("found: %hhu!", numbers[j]);
            }
        }
        ptr += 5;
    }
    
    // Show our results to the world
    //for (int j = 0; j < NUM_VALUES; j++) {
    //    printf("%lld !!\n", candidates[j]);
    //}
    
    // Clean up after ourselves
    gcl_free(cl_candidates); gcl_free(cl_keys); gcl_free(cl_results);
    free(candidates); free(hashes);
    dispatch_release(queue);
    
    // And exit.
    return 0;

}

