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

void doit(cl_ulong start, cl_ulong num_values, PAN *set) {
    
    // Create out dispatch queue, prefer GPU but allow fallback
    dispatch_queue_t queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_CPU, NULL);
    if (queue == NULL) {
        queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_CPU, NULL);
    }
    
    /**
     Perform our luhn checks, setting anything to 0 that doesn't pass
     COOLNOTE: We actually don't need to generate the numbers to use beforehand,
     because we can use the OpenCL index (gid) to construct the number on the fly as
     long as we have the start number
     */
    cl_ushort* checkbits = (cl_ushort*)malloc(sizeof(cl_ushort) * num_values);
    cl_ulong* hashes = (cl_ulong*)malloc(sizeof(cl_ulong) * num_values);
    void* cl_checkbits = gcl_malloc(sizeof(cl_ushort) * num_values, checkbits, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
    void* cl_results = gcl_malloc(sizeof(cl_ulong) * num_values, NULL, CL_MEM_WRITE_ONLY);
    dispatch_sync(queue, ^{
        cl_ndrange range = {1, {0, 0}, {num_values, 0}, {0, 0}};
        luhn_append_kernel(&range, start, cl_checkbits);
        sha1_crypt_kernel_kernel(&range, start, cl_checkbits, cl_results);
        gcl_memcpy(checkbits, cl_checkbits, num_values * sizeof(cl_ushort));
        gcl_memcpy(hashes, cl_results, num_values * sizeof(cl_ulong));
    });
    
    // we've got ourselves the keys, and now we can go through them looking for matches
    for (int i = 0; i < num_values; i++) {
        if (johnset_exists(set, hashes[i]) != 0) {
            printf("found: %llu\n", (start + i) * 10 + checkbits[i]);
        }
    }
    
    // Clean up after ourselves
    gcl_free(cl_checkbits); gcl_free(cl_results);
    free(checkbits); free(hashes);
    dispatch_release(queue);
    
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
    for (int i = 0; i < line_count; i++) {
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
    cl_ulong start = iin * 1000000000L;
    for (int i = 0; i < 10; i++) {
        long step = 1024 * 1024 * 100; // multiple of WG, up to 1bn
        doit(start + i * step, step, lookup_set);
        printf(".");
    }
}

int main(int argc, const char * argv[]) {
    
    PAN *lookup_set = construct_pan_lookup_set();
    check_iin(515279, lookup_set);
    johnset_free(lookup_set);
    
    return 0;
    
}