#include <stdio.h>
#include <stdlib.h> 
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#define MODE 256
#include "utility.h"
int STOP = 0;
int NOT_DONE = 1;
struct SumArgs{
    uint64_t start;
    uint64_t end;
    uint8_t good_key[KEY_BYTE];
    uint8_t good_nonce[NONCE_BYTE];
    int found_key;
};
typedef struct SumArgs SumArgs;

pthread_mutex_t mutex;


void apply( const unsigned char *key, 
		    const unsigned char *iv,
		    uint8_t *ciphertext){
	uint8_t *nsec;
	uint8_t *ad;
	unsigned long long adlen = 0;
	unsigned long long mlen = 4;
	uint8_t m[4] = {0x00, 0x00, 0x00, 0x00};
	
	
	unsigned long long clen;

	crypto_aead_encrypt(ciphertext, &clen, m, 4, ad, 0, nsec, iv, key);
}

int check_tag_diff(uint8_t *output_state1, uint8_t *output_state2, 
                      uint8_t *expected_diff){
	int flag = 1;
	//printf("Expected Diff :");
	//printreg(expected_diff, STATE_BYTE);
	for(int i=0; i<12; i++){
		uint32_t diff = output_state1[i] ^ output_state2[i]; 
		if(diff != expected_diff[i]){
			flag = 0;
			break;
		}
	}
	return flag;
}


void find_message(uint8_t *key1, uint8_t *nonce1, int data){
    uint64_t cube_size = (1ULL << data);
     
    uint8_t expected_diff[12] = {0x00, 0x00, 0x00, 0x00, 
                                 0x00, 0x00, 0x00, 0x00, 
                                 0x00, 0x00, 0x00, 0x00};
    
    uint8_t *nsec;
	uint8_t *ad;
	unsigned long long adlen = 0;
	
	unsigned long long mlen = 4;
	uint8_t m[4];
	
	
	unsigned long long clen;
	uint8_t c1[4+8];
	uint8_t c2[4+8];
	
	//bit position to add input key diff
    int diff_pos[4] = {127+0, 127+37, 127+81, 127+128};
    uint8_t input_key_diff[KEY_BYTE];
    get_related_key_diff(diff_pos, 4, input_key_diff);
    uint8_t key2[KEY_BYTE];
    for(int j=0; j<KEY_BYTE; j++){
    	key2[j] = key1[j] ^ input_key_diff[j];
    }
    //bit position to add nonce diff
    int nonce_diff_pos[3] = {31, 63, 95};
    uint8_t nonce_diff[NONCE_BYTE];
    get_nonce_diff(nonce_diff_pos, 3, nonce_diff);
    uint8_t nonce2[NONCE_BYTE];
    for(int j=0; j<NONCE_BYTE; j++){
    	nonce2[j] = nonce1[j] ^ nonce_diff[j];
    }
    
    for(uint64_t i=0; i < cube_size; i++){
    	generate_random_state_or_key(m, 4);
    	crypto_aead_encrypt(c1, &clen, m, 4, ad, 0, nsec, nonce1, key1);
    	crypto_aead_encrypt(c2, &clen, m, 4, ad, 0, nsec, nonce2, key2);
    	
    	    //printf("Message:\n");
    	    //printreg(m, 4);
    	    /*
    	    printf("Key:\n");
        	printreg(key1, KEY_BYTE);
        	printreg(key2, KEY_BYTE);
        	printf("Nonce:\n");
        	printreg(nonce1, NONCE_BYTE);
        	printreg(nonce2, NONCE_BYTE);
        	*/
        	//printf("C + TAG:\n");
        	//printreg(c1, 12);
        	//printreg(c2, 12);
        	
    	//print_info(output1, output2, key1, key2);
        if (check_tag_diff(c1, c2, expected_diff) == 1){
        	printf("%ld: ============TAG MATCH=============\n", i);
        	printf("Message:\n");
    	    printreg(m, 4);
        	printf("Key:\n");
        	printreg(key1, KEY_BYTE);
        	printreg(key2, KEY_BYTE);
        	printf("Nonce:\n");
        	printreg(nonce1, NONCE_BYTE);
        	printreg(nonce2, NONCE_BYTE);
        	printf("C + TAG:\n");
        	printreg(c1, 12);
        	printreg(c2, 12);
        	NOT_DONE = 0;
        	//scanf("continue?\n");
        	break;
        }
        
    }
}

int check_output_diff(uint8_t *output1, uint8_t *output2){
	int flag = 1;
	for(int i=0; i<4; i++){
		if(output1[i] != output2[i]){
			flag = 0;
			break;
		}
	}
	return flag;
}
void* encrypt_over_ranges(void *args){
    SumArgs *sumargs = (SumArgs*) args;
    
	//bit position to add input key diff
    int diff_pos[4] = {127+0, 127+37, 127+81, 127+128};
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

    uint8_t output1[4+8];
	uint8_t output2[4+8];
	
    for(uint64_t i=sumargs->start; i < sumargs->end && STOP == 0; i++){
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
    	
    	/*
    	pthread_mutex_lock(&mutex);
        printf("Key:\n");
    	printreg(key1, KEY_BYTE);
    	printreg(key2, KEY_BYTE);
    	printf("Nonce:\n");
    	printreg(nonce1, NONCE_BYTE);
    	printreg(nonce2, NONCE_BYTE);
    	printf("Output:\n");
    	printreg(output1, 12);
    	printreg(output2, 12);
        pthread_mutex_unlock(&mutex);
        */
        
        if (check_output_diff(output1, output2) == 1){
            pthread_mutex_lock(&mutex);
        	printf("%ld: ========Found Good Keys Nonces========\n", i);
        	printf("Key:\n");
        	printreg(key1, KEY_BYTE);
        	printreg(key2, KEY_BYTE);
        	printf("Nonce:\n");
        	printreg(nonce1, NONCE_BYTE);
        	printreg(nonce2, NONCE_BYTE);
        	printf("Output:\n");
        	printreg(output1, 12);
        	printreg(output2, 12);
        	for(int j=0; j<KEY_BYTE; j++){
        	    (sumargs->good_key)[j] = key1[j];
        	}
        	for(int j=0; j<NONCE_BYTE; j++){
        	    (sumargs->good_nonce)[j] = nonce1[j];
        	}
        	sumargs->found_key = 1;
        	STOP = 1;
	    	pthread_mutex_unlock(&mutex);
        }
        
    }
    pthread_exit(NULL);
}

int apply_related_key_threaded(uint8_t *key, uint8_t *nonce,
                        int log_of_key_nonce){
    pthread_t thread_ids[NROF_THREADS]; 
    SumArgs thread_args[NROF_THREADS];
   
    uint64_t cube_size = (1ULL << log_of_key_nonce);
	uint64_t data_in_each_thread = (cube_size / NROF_THREADS);
	
    for(int i=0; i < NROF_THREADS; i++){
        thread_args[i].start = i * data_in_each_thread;
        thread_args[i].end = (i+1) * data_in_each_thread;
        thread_args[i].found_key = 0;
    }

    for(int i=0; i < NROF_THREADS; i++){
        pthread_create(thread_ids + i, NULL, encrypt_over_ranges, (void*) (thread_args + i));
    }
    for(int i=0; i < NROF_THREADS; i++){
        pthread_join(thread_ids[i], NULL);
    }
    for(int i=0; i < NROF_THREADS; i++){
    	if(thread_args[i].found_key == 1){
    	    for(int j=0; j<KEY_BYTE; j++){
        	    key[j] = thread_args[i].good_key[j];
        	}
        	for(int j=0; j<NONCE_BYTE; j++){
        	    nonce[j] = thread_args[i].good_nonce[j];
        	}
        	return 1;
    	}
    }
    return 0;
}
void test_related_key(){
    int log_of_key_nonce = 33;
    int log_of_message = 16;
    uint8_t key[KEY_BYTE];
    uint8_t nonce[NONCE_BYTE];
    while(NOT_DONE){
        xx_initialize(SEED);
        printreg(SEED, 32);
        if(apply_related_key_threaded(key, nonce, log_of_key_nonce)){
            printf("Finding Messages \n");
            find_message(key, nonce, log_of_message);
        }
    }
}

//Vary over Keys only
int main(){
	test_related_key();
}
