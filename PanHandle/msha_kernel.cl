/**
 This is the SHA1 reference implementation wrapped in an OpenCL 
 kernel I wrote.  The meat of the modifications here are at the end
 */

#define SHA1CircularShift(bits,word) ((((word) << (bits)) & 0xFFFFFFFF) | ((word) >> (32-(bits))))

typedef struct SHA1Context{
	unsigned Message_Digest[5];
	unsigned Length_Low;
	unsigned Length_High;
	unsigned char Message_Block[64];
	int Message_Block_Index;
	int Computed;
	int Corrupted;
} SHA1Context;

void SHA1Reset(SHA1Context *);
int SHA1Result(SHA1Context *);
void SHA1Input( SHA1Context *,const char *,unsigned);

/* implementation */

void SHA1ProcessMessageBlock(SHA1Context *);
void SHA1PadMessage(SHA1Context *);

void SHA1Reset(SHA1Context *context){// ³õÊ¼»¯¶¯×÷
	context->Length_Low             = 0;
	context->Length_High            = 0;
	context->Message_Block_Index    = 0;
    
	context->Message_Digest[0]      = 0x67452301;
	context->Message_Digest[1]      = 0xEFCDAB89;
	context->Message_Digest[2]      = 0x98BADCFE;
	context->Message_Digest[3]      = 0x10325476;
	context->Message_Digest[4]      = 0xC3D2E1F0;
    
	context->Computed   = 0;
	context->Corrupted  = 0;
}

int SHA1Result(SHA1Context *context){// ³É¹¦·µ»Ø1£¬Ê§°Ü·µ»Ø0
	if (context->Corrupted) {
		return 0;
	}
	if (!context->Computed) {
		SHA1PadMessage(context);
		context->Computed = 1;
	}
	return 1;
}

void SHA1Input(SHA1Context *context,const char *message_array,unsigned length){
	if (!length) return;
    
	if (context->Computed || context->Corrupted){
		context->Corrupted = 1;
		return;
	}
    
	while(length-- && !context->Corrupted){
		context->Message_Block[context->Message_Block_Index++] = (*message_array & 0xFF);
        
		context->Length_Low += 8;
        
		context->Length_Low &= 0xFFFFFFFF;
		if (context->Length_Low == 0){
			context->Length_High++;
			context->Length_High &= 0xFFFFFFFF;
			if (context->Length_High == 0) context->Corrupted = 1;
		}
        
		if (context->Message_Block_Index == 64){
			SHA1ProcessMessageBlock(context);
		}
		message_array++;
	}
}

void SHA1ProcessMessageBlock(SHA1Context *context){
	const unsigned K[] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };
	int         t;
	unsigned    temp;
	unsigned    W[80];
	unsigned    A, B, C, D, E;
    
	for(t = 0; t < 16; t++) {
        W[t] = ((unsigned) context->Message_Block[t * 4]) << 24;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 1]) << 16;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 2]) << 8;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 3]);
	}
	
	for(t = 16; t < 80; t++)  W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    
	A = context->Message_Digest[0];
	B = context->Message_Digest[1];
	C = context->Message_Digest[2];
	D = context->Message_Digest[3];
	E = context->Message_Digest[4];
    
	for(t = 0; t < 20; t++) {
		temp =  SHA1CircularShift(5,A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}
	for(t = 20; t < 40; t++) {
		temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}
	for(t = 40; t < 60; t++) {
		temp = SHA1CircularShift(5,A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}
	for(t = 60; t < 80; t++) {
		temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = SHA1CircularShift(30,B);
		B = A;
		A = temp;
	}
	context->Message_Digest[0] = (context->Message_Digest[0] + A) & 0xFFFFFFFF;
	context->Message_Digest[1] = (context->Message_Digest[1] + B) & 0xFFFFFFFF;
	context->Message_Digest[2] = (context->Message_Digest[2] + C) & 0xFFFFFFFF;
	context->Message_Digest[3] = (context->Message_Digest[3] + D) & 0xFFFFFFFF;
	context->Message_Digest[4] = (context->Message_Digest[4] + E) & 0xFFFFFFFF;
	context->Message_Block_Index = 0;
}

void SHA1PadMessage(SHA1Context *context){
	if (context->Message_Block_Index > 55) {
		context->Message_Block[context->Message_Block_Index++] = 0x80;
		while(context->Message_Block_Index < 64)  context->Message_Block[context->Message_Block_Index++] = 0;
		SHA1ProcessMessageBlock(context);
		while(context->Message_Block_Index < 56) context->Message_Block[context->Message_Block_Index++] = 0;
	} else {
		context->Message_Block[context->Message_Block_Index++] = 0x80;
		while(context->Message_Block_Index < 56) context->Message_Block[context->Message_Block_Index++] = 0;
	}
	context->Message_Block[56] = (context->Length_High >> 24 ) & 0xFF;
	context->Message_Block[57] = (context->Length_High >> 16 ) & 0xFF;
	context->Message_Block[58] = (context->Length_High >> 8 ) & 0xFF;
	context->Message_Block[59] = (context->Length_High) & 0xFF;
	context->Message_Block[60] = (context->Length_Low >> 24 ) & 0xFF;
	context->Message_Block[61] = (context->Length_Low >> 16 ) & 0xFF;
	context->Message_Block[62] = (context->Length_Low >> 8 ) & 0xFF;
	context->Message_Block[63] = (context->Length_Low) & 0xFF;
    
	SHA1ProcessMessageBlock(context);
}

/**
 This kernel will take a checkbit, and for reach position, compute a SHA1 of the credit
 card number that is at that position.  Once it has it, to save memory moving back and
 forth, it will go ahead and hash the value it has into a ulong which it will return
 to check against the hashset
 */
__kernel void sha1_crypt_kernel(const unsigned long base, __global ushort* bits, __global unsigned long* hashes)
{
    uint message_length = 16;
    uint gid = get_global_id(0);

    // the number in question
    unsigned long num = (base + gid) * 10 + bits[gid];
    
    // build a char array
    char temp[message_length];
    sprintf(&temp, (const char *)"%lu", num); // lu instead of luu due to opencl being opencl: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/printfFunction.html
    
    // perform hashing
    SHA1Context context;
    SHA1Reset(&context);
    SHA1Input(&context, temp, message_length);
    SHA1Result(&context);
    
    // copy to our output
    // NOTE: answer doesn't need to necessarily be built up
    char answer[20];
    char* cs = (char*) context.Message_Digest;
    for (int i = 0; i < 5; i++) {
        answer[i * 4 + 3] = cs[i * 4 + 0];
        answer[i * 4 + 2] = cs[i * 4 + 1];
        answer[i * 4 + 1] = cs[i * 4 + 2];
        answer[i * 4 + 0] = cs[i * 4 + 3];
    }
    
    // turn that guy into a hash while we're sitting here looking at it
    unsigned long hash = 5381;
    int c;
    for (int i = 0; i < 20; i++) {
        c = answer[i];
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    hashes[gid] = hash;
    
}
