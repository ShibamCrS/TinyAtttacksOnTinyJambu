#include "configure.h"
#include "../xoshiro256plusplus.h"
uint64_t SEED[4];

void printreg_to_file(const void *a, int nrof_byte, FILE *fp){
    int i;
    unsigned char *f = (unsigned char *)a;
    for(i=0; i < nrof_byte; i++){
        fprintf(fp, "%02x, ",(unsigned char) f[i]); //uint8_t c[4+8];
	unsigned long long clen;
    }
    fprintf(fp, "\n");
}
void printreg(const void *a, int nrof_byte){
    printreg_to_file(a, nrof_byte, stdout);
}

void xor_32(uint32_t *source1, uint32_t *source2, uint32_t *dest){
	for (int i=0; i<4; i++){
		dest[i] = source1[i] ^ source2[i];
	}
}
void xor_8(uint8_t *source1, uint8_t *source2, uint8_t *dest){
	for (int i=0; i<12; i++){
		dest[i] = source1[i] ^ source2[i];
	}
}
void generate_random_state_or_key(uint8_t *random_data, int nrof_byte){
    int required_call = nrof_byte/8 + 1;
    uint64_t *random64 = (uint64_t *)malloc(required_call*sizeof(uint64_t));
    for(int i=0; i<required_call; i++){
        random64[i] = xx_next(SEED);
    }
    memcpy(random_data, random64, nrof_byte);
    free(random64);
}
/*
void generate_random_state_or_key(uint8_t *random_data, int nrof_byte){
		for(int i=0; i<nrof_byte; i++){
			uint8_t a = rand() % 256;
			random_data[i] = a;
		}
}
*/

void get_related_key_diff(int *pos, int nrof_pos, uint8_t *input_key_diff){
	for(int i=0; i<KEY_BYTE; i++){
		input_key_diff[i] = 0x00;
	}
	
    for(int i=0; i < nrof_pos; i++){
    	int byte_pos = (pos[i] / 8);
    	int bit_pos  = pos[i] % 8;
    	input_key_diff[byte_pos] |= (1 << bit_pos); 
    }
    //printf("Input Key Diff :");
	//printreg192(input_key_diff);
}

void get_nonce_diff(int *pos, int nrof_pos, uint8_t *nonce_diff){
	for(int i=0; i<NONCE_BYTE; i++){
		nonce_diff[i] = 0x00;
	}
	
    for(int i=0; i < nrof_pos; i++){
    	int byte_pos = (pos[i] / 8);
    	int bit_pos  = pos[i] % 8;
    	nonce_diff[byte_pos] |= (1 << bit_pos); 
    }
    //printf("Input Key Diff :");
	//printreg192(input_key_diff);
}

void print_info(uint32_t *output1, uint32_t *output2, 
                uint8_t *key1, uint8_t *key2){
    printf("key\n");
    printreg(key1, KEY_BYTE);
    printreg(key2, KEY_BYTE);
	printf("output\n");
	printreg(output1, STATE_BYTE);
	printreg(output2, STATE_BYTE);
	printf("Output Difference\n");
	uint32_t diff[4];
	xor_32(output1, output2, diff);
	printreg(diff, STATE_BYTE);
}

void print_info_mode_to_file(uint32_t *output1, uint32_t *output2, 
                uint8_t *key1, uint8_t *key2, 
                uint8_t *nonce1, uint8_t *nonce2,
                FILE *fp){
    fprintf(fp, "key\n");
    printreg_to_file(key1, KEY_BYTE, fp);
    printreg_to_file(key2, KEY_BYTE, fp);
	fprintf(fp, "output\n");
	printreg_to_file(output1, STATE_BYTE, fp);
	printreg_to_file(output2, STATE_BYTE, fp);
	fprintf(fp, "Output Difference\n");
	uint32_t diff[4];
	xor_32(output1, output2, diff);
	printreg_to_file(diff, STATE_BYTE, fp);
	fprintf(fp, "nonce\n");
	printreg_to_file(nonce1, NONCE_BYTE, fp);
	printreg_to_file(nonce2, NONCE_BYTE, fp);
	fprintf(fp, "nonce Difference\n");
	uint8_t nonce_diff[NONCE_BYTE];
	xor_8(nonce1, nonce2, nonce_diff);
	printreg_to_file(nonce_diff, NONCE_BYTE, fp);
}
void print_info_mode(uint32_t *output1, uint32_t *output2, 
                uint8_t *key1, uint8_t *key2, 
                uint8_t *nonce1, uint8_t *nonce2){
    print_info_mode_to_file(output1, output2, key1, key2, nonce1, nonce2, stdout);             
}
void calculateSD(uint64_t *data) {
    uint64_t sum = 0UL;
    long double mean;
    long double sd = 0.0;

    int i;
    for (i = 0; i < EXP; i++){
        sum += data[i];
    }
    mean = ( (long double)sum) /( (long double)EXP);
    for (i = 0; i < EXP; i++){
        long double temp = ((long double)(data[i])) - mean;
        sd += pow(temp, 2);
    }
    sd = sd/((long double)EXP);
    sd = sqrt(sd);
    mean = mean / ((long double)(1UL << LOK));
    sd   =   sd / ((long double)(1UL<<LOK));
    long double lmean = logl(mean)/logl(2.0);
    long double lsd = logl(sd)/logl(2.0);
    printf("AVERAGE PROBABILITY: (2^%5.3Lf) AVERAGE SD: (2^%5.3Lf)\n",lmean, lsd);

}
