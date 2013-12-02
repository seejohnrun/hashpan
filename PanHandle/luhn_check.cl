// This is cool because it makes it so that we only have to look at the numbers
// which actually pass the luhn test
__kernel void luhn_append(const ulong base, __global ulong *candidates) {
    
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
    
    candidates[gid] = check;
    
}