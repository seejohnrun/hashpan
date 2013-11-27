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

void doit(unsigned long start, unsigned long num_values) {
    
    // read in all lines
    char lines[1022][29];
    FILE *fp = fopen("/Users/john/Development/PanHandle/data/hashes.txt", "r");
    for (int i = 0; i < 1022; i++) { // TODO flex
        fgets(lines[i], 29, fp);
    }
    fclose(fp);
    
    // decode each line
    // TODO note for why i did this here
    cl_uchar choices[1022][20];
    for (int i = 0; i < 1022; i++) { // TODO flex
        Base64decode((void *)choices[i], lines[i]);
    }
        
    // Create out dispatch queue, prefer GPU but allow fallback
    dispatch_queue_t queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_GPU, NULL);
    if (queue == NULL) {
        queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_CPU, NULL);
    }

    dispatch_queue_t queue2 = gcl_create_dispatch_queue(CL_DEVICE_TYPE_CPU, NULL);
    if (queue2 == NULL) {
        queue2 = gcl_create_dispatch_queue(CL_DEVICE_TYPE_GPU, NULL);
    }
    
    // Put together our test data
    cl_ulong* candidates = (cl_ulong*)malloc(sizeof(cl_ulong) * num_values); // TODO reuse this array on subsequent runs
    for (int i = 0; i < num_values; i++) {
        candidates[i] = start + i;
    }
    printf("starting with %ld at %ld\n", num_values, start);
    
    // perform our luhn checks, setting anything to 0 that doesn't pass
    void* cl_candidates = gcl_malloc(sizeof(cl_ulong) * num_values, candidates, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
    dispatch_sync(queue, ^{
        size_t wgs;
        gcl_get_kernel_block_workgroup_info(luhn_check_kernel, CL_KERNEL_WORK_GROUP_SIZE, sizeof(wgs), &wgs, NULL);
        cl_ndrange range = {1, {0, 0, 0}, {num_values, 0, 0}, {wgs, 0, 0}};
        luhn_check_kernel(&range, (cl_ulong*)cl_candidates);
        gcl_memcpy(candidates, cl_candidates, sizeof(cl_ulong) * num_values);
    });
    
    // count the number of remaining numbers
    unsigned int count = 0;
    for (int j = 0; j < num_values; j++) {
        if (candidates[j] != 0) {
            count += 1;
        }
    }
    printf("left with %d after luhn\n", count);
    
    // and convert the leftovers into a char array
    cl_uchar numbers[count * 16 * sizeof(cl_uchar)];
    void *ptr = numbers;
    for (int j = 0; j < num_values; j++) {
        if (candidates[j] != 0) {
            sprintf(ptr, "%lld ", candidates[j]);
            ptr += sizeof(cl_uchar) * 16;
        }
    }
        
    // now we're ready to SHA1 the keys
    short key_length = 5;
    void* cl_keys = gcl_malloc(sizeof(cl_uchar) * count * 16, numbers, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_results = gcl_malloc(sizeof(cl_uint) * count * key_length, NULL, CL_MEM_WRITE_ONLY);
    void* hashes = malloc(sizeof(cl_uint) * count * key_length);
    dispatch_sync(queue2, ^{
        size_t wgs;
        gcl_get_kernel_block_workgroup_info(sha1_crypt_kernel_kernel, CL_KERNEL_WORK_GROUP_SIZE, sizeof(wgs), &wgs, NULL);
        cl_ndrange range = {1, {0, 0, 0}, {count, 0, 0}, {wgs, 0, 0}};
        sha1_crypt_kernel_kernel(&range, (cl_uint*)cl_keys, (cl_uint*)cl_results);
        gcl_memcpy(hashes, cl_results, sizeof(cl_uint) * count * key_length);
    });
    printf("produced SHA\n");
    
    // we've got outselves the keys, and now we can go through them looking for matches
    // TODO use hashmap of some sort for comparison
    for (int i = 0; i < 1022; i++) {
        void *ptr = hashes;
        for (int j = 0; j < count; j++) {
            
            cl_uchar temp[16];
            void *pre = numbers;
            pre += j * sizeof(cl_uchar);
            memcpy(temp, pre, 16 * sizeof(cl_uchar));
            printf("number: %s\n", temp);
            
            cl_uchar sha[sizeof(cl_uint) * key_length];
            pre = hashes;
            pre += j * sizeof(cl_uint) * key_length;
            memcpy(sha, pre, sizeof(cl_uint) * key_length);
            printf("sha: %s\n", sha);
            
            printf("raw: %s\n", lines[i]);
            printf("com: %s\n", choices[i]);
            
            if (memcmp(choices[i], ptr, sizeof(cl_uint) * key_length) == 0) {
                printf("found: %hhu!\n", numbers[j]);
            }

        }
        ptr += key_length * sizeof(cl_uint);
    }
    
    // Clean up after ourselves
    gcl_free(cl_candidates); gcl_free(cl_keys); gcl_free(cl_results);
    free(candidates); free(hashes);
    dispatch_release(queue);
    dispatch_release(queue2);
    
}

int main(int argc, const char * argv[]) {
    
    long start = 4242424242424242;
    long step = 1024;
    for (int i = 0; i < 1; i++) {
        doit(start, step);
        start += step;
    }
    
    return 0;
    
}