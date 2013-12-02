// This is cool because it makes it so that we only have to look at the numbers
// which actually pass the luhn test.  We don't even pass the number bases into here,
// but rather compute those on the fly as we generate the checkbits.  That means
// we're never stuck building up large arrays in memory just run over them.
__kernel void luhn_append(const ulong base, __global ushort *candidates) {
    
    size_t gid = get_global_id(0);
    ulong num = base + gid;
    
    // compute the luhn sans checkbit
    short dbl = 1;
    short sum = 0;
    short digit;
    while (num > 0) {
        digit = num % 10;
        if (dbl) {
            digit *= 2;
            if (digit > 9) {
                digit -= 9;
            }
        }
        sum += digit;
        num /= 10;
        dbl = !dbl;
    }
    
    // get the check digit, and build the full number
    short check = 10 - sum % 10;
    if (check == 10) { check = 0; }
    
    // return the checkbits for this number
    candidates[gid] = check;
    
}