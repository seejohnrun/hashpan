// TODO add note here
kernel void luhn_check(global unsigned long* candidates) {

    size_t i = get_global_id(0);
    unsigned long num = candidates[i];
    
    short digit;
    short sum = 0;
    short dbl = 0; // false
    
    while (num > 0) {

        digit = num % 10;
        
        if (dbl) {
            digit *= 2;
            if (digit > 9) {
                digit -= 9; // TODO add note here
            }
        }
        
        sum += digit;
        dbl ^= dbl;
        num /= 10;
        
    }

    if (sum % 10 != 0) {
        candidates[i] = 0; // TODO add note here
    }

}