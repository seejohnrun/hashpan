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
#include "hashset.h"
#include "luhn_check.cl.h"
#include "msha_kernel.cl.h"

// djb2
unsigned long hash_c(unsigned char *str) {
    printf("%s\n", str);
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

void doit(unsigned long start, unsigned long num_values, hashset_t set) {
    
    printf("start\n");
    
    
    // Create out dispatch queue, prefer GPU but allow fallback
    dispatch_queue_t queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_GPU, NULL);
    if (queue == NULL) {
        queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_CPU, NULL);
    }

    dispatch_queue_t queue2 = gcl_create_dispatch_queue(CL_DEVICE_TYPE_GPU, NULL);
    if (queue2 == NULL) {
        queue2 = gcl_create_dispatch_queue(CL_DEVICE_TYPE_CPU, NULL);
    }
    
    /**
     Perform our luhn checks, setting anything to 0 that doesn't pass
     COOLNOTE: We actually don't need to generate the numbers to use beforehand,
     because we can use the OpenCL index (gid) to construct the number on the fly as
     long as we have the start number
     */
    printf("starting with %ld at %ld\n", num_values, start);
    cl_ulong* candidates = (cl_ulong*)malloc(sizeof(cl_ulong) * num_values); // TODO reuse this array on subsequent runs
    void* cl_candidates = gcl_malloc(sizeof(cl_ulong) * num_values, candidates, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
    dispatch_sync(queue, ^{
        size_t wgs;
        gcl_get_kernel_block_workgroup_info(luhn_check_kernel, CL_KERNEL_WORK_GROUP_SIZE, sizeof(wgs), &wgs, NULL);
        cl_ndrange range = {1, {0, 0, 0}, {num_values, 0, 0}, {wgs, 0, 0}};
        luhn_check_kernel(&range, start, (cl_ulong*)cl_candidates);
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
    cl_uchar numbers[count * 16];
    void *ptr = numbers;
    for (int j = 0; j < num_values; j++) {
        if (candidates[j] != 0) {
            sprintf(ptr, "%lld", candidates[j]);
            ptr += sizeof(cl_uchar) * 16;
        }
    }
    
    // now we're ready to SHA1 the keys
    short hash_length = 5 * sizeof(cl_uint); // 160 bits
    void* cl_keys = gcl_malloc(sizeof(cl_uchar) * count * 16, numbers, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_results = gcl_malloc(count * hash_length, NULL, CL_MEM_WRITE_ONLY);
    cl_uchar* hashes = (cl_uchar*)malloc(hash_length * count);
    dispatch_sync(queue2, ^{
        cl_ndrange range = {1, {0, 0, 0}, {count, 0, 0}, {NULL, 0, 0}};
        sha1_crypt_kernel_kernel(&range, cl_keys, cl_results);
        gcl_memcpy(hashes, cl_results, count * hash_length);
    });
    printf("produced SHA\n");
    
    // we've got outselves the keys, and now we can go through them looking for matches
    cl_uchar tmp[hash_length + 1];
    for (int i = 0; i < count; i++) {
        memcpy(tmp, &hashes[i * hash_length], hash_length);
        tmp[hash_length] = 0; // terminate that biz
        if (hashset_is_member(set, &tmp)) {
            printf("found: %hhu!\n", numbers[i]);
        }
    }
    
    // Clean up after ourselves
    gcl_free(cl_candidates); gcl_free(cl_keys); gcl_free(cl_results);
    free(candidates); free(hashes);
    dispatch_release(queue);
    dispatch_release(queue2);
    
}

int main(int argc, const char * argv[]) {
    
    struct pan {
        char hash[20];
        UT_hash_handle hh;
    };
    
    // read in all lines
    int line_count = 1022;
    char lines[line_count][29];
    FILE *fp = fopen("/Users/john/Development/PanHandle/data/hashes.txt", "r");
    for (int i = 0; i < line_count; i++) { // TODO flex
        fgets(lines[i], 29, fp);
    }
    fclose(fp);
    
    // decode each line
    // TODO note for why i did this here
    cl_char temp[20];
    struct pan *pans = NULL, *pan;
    for (int i = 0; i < line_count; i++) { // TODO flex
        Base64decode((void *)temp, lines[i]);
        pan->hash = temp;
        HASH_ADD_STR(pans, hash, pan);
    }

    
    
    
    long start = 4242424242424242;
    long step = 1024 * 1024;
    for (int i = 0; i < 1; i++) {
        doit(start, step, pans);
        start += step;
    }
    
    free(pans);
    
    return 0;
    
}