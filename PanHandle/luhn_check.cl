// This luhn check kernel starts with a base, and fills in only the numbers that
// pass luhn check (leaving the rest set at 0).  it's a quick luhn implementation
// built to run on OpenCL
__kernel void luhn_check(const unsigned long base, __global unsigned long *candidates) {
    
    size_t gid = get_global_id(0);
    unsigned long num = base + gid;
    
    // If one of the numbers that is in the same %10 as this number was valid,
    // then this number can't be valid - so we won't bother running it.  This
    // pre-checked saves us a lot of luhn checks
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
                // since 2x any number will always start with a 1, we can add
                // the digits by just subtracting 9 (ie, 12 -> 3, 18 -> 9)
                digit -= 9;
            }
        }
        
        sum += digit;
        dbl = !dbl;
        num /= 10;
        
    }
    
    // successful luhn check, set to be this number
    if (sum % 10 == 0) {
        candidates[gid] = base + gid;
    }
    
}