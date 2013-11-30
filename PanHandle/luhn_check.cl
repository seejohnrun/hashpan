__kernel void luhn_check(const unsigned long base, __global unsigned long *candidates) {
    
    size_t gid = get_global_id(0);
    unsigned long num = base + gid;
    
    // TODO add note here
    unsigned long rem = num % 10;
    for (short i = 0; i < rem; i++) {
        if (i >= gid && candidates[gid - i] != 0) {
            return; // failed luhn
        }
    }
    
    short digit;
    short sum = 0;
    short dbl = 0; // true, starting at right
    
    while (num > 0) {
        
        digit = num % 10;
        
        if (dbl) {
            digit *= 2;
            if (digit > 9) {
                digit -= 9; // TODO add note here
            }
        }
        
        sum += digit;
        dbl = !dbl;
        num /= 10;
        
    }
    
    // successful luhn check
    if (sum % 10 == 0) {
        candidates[gid] = base + gid;
    }
    
}