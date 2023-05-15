#include <stdio.h>
#include <stdlib.h> 
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#define MODE 128
#define LOK 20
#define EXP 50
#include "utility.h"


struct SumArgs{
    uint8_t *key;
    uint64_t start;
    uint64_t end;
    int log_data;
    int rounds;
    uint32_t *output_diff_list;
    uint64_t nrof_hit;
};

typedef struct SumArgs SumArgs;
uint64_t TOTAL_DATA = 0;
uint64_t HIT = 0;

pthread_mutex_t mutex;

void apply(const unsigned int *inputs, 
		   const unsigned char *key, 
		         unsigned int number_of_steps,
		         unsigned int *outputs){
	for(int i=0; i<4; i++){
		outputs[i] = inputs[i];
	}
	state_update(outputs, key, number_of_steps);	      
}

/* void get_related_key_diff(int *pos, int nrof_pos, uint8_t *input_key_diff){ */
/* 	for(int i=0; i<16; i++){ */
/* 		input_key_diff[i] = 0x00; */
/* 	} */
	
/*     for(int i=0; i < nrof_pos; i++){ */
/*     	int byte_pos = (pos[i] / 8); */
/*     	int bit_pos  = pos[i] % 8; */
/*     	input_key_diff[byte_pos] |= (1 << bit_pos); */ 
/*     } */
/*     //printf("Input Key Diff :"); */
/* 	//printreg(input_key_diff); */
/* } */

int check_output_diff(uint32_t *output_state1, uint32_t *output_state2){
	int flag = 1;
	uint32_t expected_diff[4] = {0x00000000, 0x00000000, 0x00000000, 0x80000000};
	//printf("Expected Diff :");
	//printreg(expected_diff);
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

	uint8_t state_bytes[16];
    
    uint32_t state2[4];

	uint32_t output1[4];
	uint32_t output2[4];
	
	//input plaintext difference is at bit 127
	uint32_t input_diff[4] = {0x00000000, 0x00000000, 0x00000000, 0x80000000}; 
	
	//bit position to add input key diff
    int diff_pos[2] = {36, 80};
    uint8_t input_key_diff[16];
    get_related_key_diff(diff_pos, 2, input_key_diff);
    uint8_t key2[16];

    for(uint64_t i=sumargs->start; i < sumargs->end; i += 1){
        generate_random_state_or_key(state_bytes, STATE_BYTE);
	    uint32_t *state1 = (uint32_t *)state_bytes;
    	for(int j=0; j<4; j++){
    		state2[j] = state1[j] ^ input_diff[j];
    	}
    	for(int j=0; j<16; j++){
    		key2[j] = *(sumargs->key + j) ^ input_key_diff[j];
    	}
    	apply(state1, sumargs->key, sumargs->rounds, output1);
    	apply(state2, key2, sumargs->rounds, output2);
    	
    	pthread_mutex_lock(&mutex);
        if (check_output_diff(output1, output2) == 1){
        	sumargs->nrof_hit++;
        	
        	/* printf("%ld ============Matched=============\n",i); */
        	/* print_info(output1, output2, sumargs->key, key2); */
        }
        pthread_mutex_unlock(&mutex);
		
        /* state1[0]++; // this will not work with log_data >32 */
    }
    pthread_exit(NULL);
}

void apply_related_key_threaded(uint8_t *key, int log_data, int rounds){
    pthread_t thread_ids[NROF_THREADS]; 
    SumArgs thread_args[NROF_THREADS];
    
    uint64_t cube_size = (1ULL << log_data);
	uint64_t data_in_each_thread = (cube_size / NROF_THREADS);
	
    for(int i=0; i < NROF_THREADS; i++){
        thread_args[i].key = key;
        thread_args[i].start = i * data_in_each_thread;
        thread_args[i].end = (i+1) * data_in_each_thread;
        thread_args[i].log_data = log_data;
        thread_args[i].rounds = rounds;
        thread_args[i].output_diff_list = (uint32_t *)malloc((data_in_each_thread*4)*sizeof(uint32_t));
        thread_args[i].nrof_hit = 0;
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
}


/* void test_related_key(int log_of_data, int rounds){ */
/*     uint8_t key[16]; */
/*     for(uint64_t i=0; i < 32; i++){ */
/*     	generate_random_state_or_key(key, KEY_BYTE); */
/*         printf("\nKey: %ld\n",i); */
/*         printreg(key,KEY_BYTE); */
/*         apply_related_key_threaded(key, log_of_data, rounds); */
/*         TOTAL_DATA = TOTAL_DATA + (1ULL << log_of_data); */
/*     } */
/*     printf("(NUMBER_OF_PLAINTEXT_PAIRS_SATISFY_THE_DIFFERENCE/TOTAL PAIRS) = (%lu/%lu)\n",HIT,TOTAL_DATA); */
/*     printf("AVERAGE PROBABILITY: (2^%5.2Lf) \n", logl( (((long double) HIT)/ ((long double) TOTAL_DATA)))/logl(2.0) ); */ 
/* } */

/* int main(){ */
/* 	xx_initialize(SEED); */
/*     printreg(SEED, 32); */
/* 	int log_of_data = 24; */
/* 	int rounds = 1024; */
/*     test_related_key(log_of_data, rounds); */
/* } */

uint64_t test_related_key(){
    int log_of_data = LOK;
    int rounds = 1024;
    uint8_t key[16];
    generate_random_state_or_key(key, KEY_BYTE);
    printf("Key: ");
    printreg(key,KEY_BYTE);
    
    HIT = 0UL;
    apply_related_key_threaded(key, log_of_data, rounds);

    TOTAL_DATA = (1ULL << log_of_data);
    printf("(NUMBER_OF_PLAINTEXT_PAIRS_SATISFY_THE_DIFFERENCE/TOTAL PAIRS) = (%lu/%lu)\n",HIT,TOTAL_DATA);
    printf("AVERAGE PROBABILITY: (2^%5.2Lf) \n", logl( (((long double) HIT)/ ((long double) TOTAL_DATA)))/logl(2.0) );
    return HIT;
}

int main(){
    xx_initialize(SEED);
    printreg(SEED, 32);
    uint64_t data[EXP];
    for(int i=0; i<EXP; i++){
        data[i] = test_related_key();
    }
    calculateSD(data);
}

