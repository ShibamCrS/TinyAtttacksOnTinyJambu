#include <stdio.h>
#include <stdlib.h> 
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#define MODE 192
#include "utility.h"

struct SumArgs{
    uint64_t start;
    uint64_t end;
    int log_data;
    int rounds;
    uint32_t *expected_diff;
    uint64_t nrof_hit;
    FILE *fpdata; 
};

typedef struct SumArgs SumArgs;
uint64_t TOTAL_DATA = 0;
uint64_t HIT = 0;

pthread_mutex_t mutex;

//Call To The Oracle
void apply( const unsigned char *key, 
		    const unsigned char *iv, 
		         unsigned int *outputs){
	for(int i=0; i<4; i++){
		outputs[i] = 0;
	}
	//state_update(outputs, key, NROUND2);
	initialization(key, iv, outputs);
	//outputs[1] ^= FrameBitsPC;  
	//state_update(outputs, key, NROUND2); 
}


void get_related_key_diff(int *pos, int nrof_pos, uint8_t *input_key_diff){
	for(int i=0; i<KEY_BYTE; i++){
		input_key_diff[i] = 0x00;
	}
	
    for(int i=0; i < nrof_pos; i++){
    	int byte_pos = (pos[i] / 8);
    	int bit_pos  = pos[i] % 8;
    	input_key_diff[byte_pos] |= (1 << bit_pos); 
    }
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
}

int check_output_diff(uint32_t *output_state1, uint32_t *output_state2, 
                      uint32_t *expected_diff){
	int flag = 1;
	for(int i=0; i<4; i++){
		uint32_t diff = output_state1[i] ^ output_state2[i]; 
		if(diff != expected_diff[i]){
			flag = 0;
			break;
		}
	}
	return flag;
}

void* encrypt_over_ranges(void *args){
    SumArgs *sumargs = (SumArgs*) args;
    
	//bit position to add input key diff
    int diff_pos[4] = {63+0, 63+37, 63+81, 63+128};
    uint8_t input_key_diff[KEY_BYTE];
    get_related_key_diff(diff_pos, 4, input_key_diff);
    uint8_t key1[KEY_BYTE];
    uint8_t key2[KEY_BYTE];
    
    //bit position to add nonce diff
    int nonce_diff_pos[3] = {31, 63, 95};
    uint8_t nonce_diff[NONCE_BYTE];
    get_nonce_diff(nonce_diff_pos, 3, nonce_diff);
    uint8_t nonce1[NONCE_BYTE];
    uint8_t nonce2[NONCE_BYTE];

    uint32_t output1[4];
	uint32_t output2[4];
	
    for(uint64_t i=sumargs->start; i < sumargs->end; i++){
    	generate_random_state_or_key(key1, KEY_BYTE);
    	for(int j=0; j<KEY_BYTE; j++){
    		key2[j] = key1[j] ^ input_key_diff[j];
    	}
    	generate_random_state_or_key(nonce1, NONCE_BYTE);
    	for(int j=0; j<NONCE_BYTE; j++){
    		nonce2[j] = nonce1[j] ^ nonce_diff[j];
    	}
    	apply(key1, nonce1, output1);
    	apply(key2, nonce2, output2);

        if (check_output_diff(output1, output2, sumargs->expected_diff) == 1){
            sumargs->nrof_hit++;
            pthread_mutex_lock(&mutex);
        	printf("%ld: ============Matched=============\n", i);
        	print_info_mode_to_file(output1, output2, key1, key2, nonce1, nonce2, sumargs->fpdata);
        	print_info_mode(output1, output2, key1, key2, nonce1, nonce2);
	    	pthread_mutex_unlock(&mutex);
        }
        
    }
    pthread_exit(NULL);
}

void apply_related_key_threaded(int log_data, uint32_t *expected_diff, int rounds){
    pthread_t thread_ids[NROF_THREADS]; 
    SumArgs thread_args[NROF_THREADS];
    
    char fname[256];
    sprintf(fname, RES_PATH, rounds);
    FILE *fp = fopen(fname, "w");
    
    uint64_t cube_size = (1ULL << log_data);
	uint64_t data_in_each_thread = (cube_size / NROF_THREADS);
	
    for(int i=0; i < NROF_THREADS; i++){
        thread_args[i].start = i * data_in_each_thread;
        thread_args[i].end = (i+1) * data_in_each_thread;
        thread_args[i].log_data = log_data;
        //thread_args[i].rounds = rounds;
        thread_args[i].expected_diff = expected_diff;
        thread_args[i].nrof_hit = 0;
        thread_args[i].fpdata = fp;
    }

    for(int i=0; i < NROF_THREADS; i++){
        pthread_create(thread_ids + i, NULL, encrypt_over_ranges, (void*) (thread_args + i));
    }
    for(int i=0; i < NROF_THREADS; i++){
        pthread_join(thread_ids[i], NULL);
    }
    for(int i=0; i < NROF_THREADS; i++){
    	HIT += thread_args[i].nrof_hit;
    }
    fclose(fp);
}


void test_related_key(){
    int log_of_key = 24;
	int rounds = 2432;
	uint32_t expected_diff[4] = {0x00000000, 0x00000000, 0x00000000, 0x00000000};
    
    apply_related_key_threaded(log_of_key, expected_diff, rounds);
    char fname[256];
    sprintf(fname, RES_PATH, rounds);
    FILE *fp = fopen(fname, "a");
    TOTAL_DATA = (1ULL << log_of_key);
    fprintf(fp, "(NUMBER_OF_PLAINTEXT_PAIRS_SATISFY_THE_DIFFERENCE/TOTAL PAIRS) = (%lu/%lu)\n",HIT,TOTAL_DATA);
    fprintf(fp, "AVERAGE PROBABILITY: (2^%5.2Lf) \n", logl( (((long double) HIT)/ ((long double) TOTAL_DATA)))/logl(2.0) ); 
    fclose(fp);
}

int main(){
	srand(time(NULL));
	test_related_key();
}
