#ifndef MODE
    #define MODE 256 
#endif

#define NROF_THREADS 16
#define STATE_BYTE 16
#define NONCE_BYTE 12

#if MODE == 128
    #include "../reference_implementation/mode_128.h"
    #define KEY_BYTE 16
    #define RES_PATH "../data/related_key_128_%d.txt"
#elif MODE == 192
    #include "../reference_implementation/mode_192.h"
    #define KEY_BYTE 24
    #define RES_PATH "../data/related_key_192_%d.txt"
#elif MODE == 256
    #include "../reference_implementation/mode_256.h"
    #define KEY_BYTE 32
    #define RES_PATH "../data/related_key_256_%d.txt"
#endif

