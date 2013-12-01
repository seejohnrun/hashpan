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
#include "johnset.h"
#include "luhn_check.cl.h"
#include "msha_kernel.cl.h"

void doit(unsigned long start, unsigned long num_values, PAN *set) {

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
    cl_ulong* candidates = (cl_ulong*)malloc(sizeof(cl_ulong) * num_values);
    void* cl_candidates = gcl_malloc(sizeof(cl_ulong) * num_values, candidates, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
    dispatch_sync(queue, ^{
        cl_ndrange range = {1, {0, 0}, {num_values, 0}, {0, 0}};
        luhn_check_kernel(&range, start, (cl_ulong*)cl_candidates);
        gcl_memcpy(candidates, cl_candidates, sizeof(cl_ulong) * num_values);
    });
    
    // count the number of remaining numbers
    unsigned long count = 0;
    for (int j = 0; j < num_values; j++) {
        if (candidates[j] != 0) {
            count += 1;
        }
    }
    
    // and convert the leftovers into a char array
    cl_char numbers[count * 16];
    void *ptr = numbers;
    for (int j = 0; j < num_values; j++) {
        if (candidates[j] != 0) {
            sprintf(ptr, "%lld", candidates[j]);
            ptr += sizeof(cl_char) * 16;
        }
    }
    
    // now we're ready to SHA1 the keys
    short hash_length = 5 * sizeof(cl_uint); // 160 bits
    void* cl_keys = gcl_malloc(sizeof(cl_char) * count * 16, numbers, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR);
    void* cl_results = gcl_malloc(count * hash_length, NULL, CL_MEM_WRITE_ONLY);
    cl_char* hashes = (cl_char*)malloc(hash_length * count);
    dispatch_sync(queue2, ^{
        cl_ndrange range = {1, {0, 0}, {count, 0}, {0, 0}};
        sha1_crypt_kernel_kernel(&range, cl_keys, cl_results);
        gcl_memcpy(hashes, cl_results, count * hash_length);
    });
    
    // we've got outselves the keys, and now we can go through them looking for matches
    char tmp[hash_length];
    char tmpname[17];
    for (int i = 0; i < count; i++) {
        memcpy(tmp, &hashes[i * hash_length], hash_length);
        if (johnset_exists(set, tmp) != 0) {
            memcpy(tmpname, &numbers[i * 16], 16);
            tmpname[16] = 0;
            printf("found: %s\n", tmpname);
        }
    }
    
    // Clean up after ourselves
    gcl_free(cl_candidates); gcl_free(cl_keys); gcl_free(cl_results);
    free(candidates); free(hashes);
    dispatch_release(queue);
    dispatch_release(queue2);
    
}

/**
 Here we construct a Set that we can use to look up whether or not
 a given hash we try is in our original set.  The lookups, they're
 almost toooo fast.  Also, we make sure to Base64decode here, so
 we're not doing it repeatedly later on
 */
PAN* construct_pan_lookup_set() {
    // read in all lines
    int line_count = 1022;
    char lines[line_count][29];
    FILE *fp = fopen("./data/hashes.txt", "r");
    for (int i = 0; i < line_count; i++) { // TODO flex
        fgets(lines[i], 29, fp);
        fgetc(fp); // skip newline
    }
    fclose(fp);
    
    // decode each line
    char temp[21];
    PAN *set = johnset_initialize();
    for (int i = 0; i < line_count; i++) {
        Base64decode(temp, lines[i]);
        johnset_add(&set, temp);
    }

    return set;
}

// check over an individual IIN
void check_iin(int iin, PAN* lookup_set) {
    long start = iin * 10000000000L;
    long step = 1024 * 1024;
    for (int i = 0; i < 1024 * 10; i++) {
        doit(start, step, lookup_set);
        start += step;
    }
}

int main(int argc, const char * argv[]) {
    
    PAN *lookup_set = construct_pan_lookup_set();
    check_iin(473055, lookup_set);
    johnset_free(lookup_set);
    
    return 0;
    
}