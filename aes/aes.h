#ifndef AES_H_
#define AES_H_
#include <stdint.h>

/**< https://github.com/DavyLandman/AESLib */

typedef enum 
{
    AES_ECB = 0,
    AES_CBC
} mode_t;

/**
* Decode AES packet
* if mode == ECB, iv will be ignored
* @param key Key
* @param data Input/Output data, Decoded data will be stored in same place
* @param mode Operation mode, currently supported ECB / CBC @see mode_t
* @param iv Initial vector for CBC
*/
void aes128_dec(uint8_t* key, uint8_t* data, mode_t mode, uint8_t* iv);


/**
* Encode AES packet
* if mode == ECB, iv will be ignored
* @param key Key
* @param data Input/Output data, Encoded data will be stored in same place
* @param mode Operation mode, currently supported ECB / CBC @see mode_t
* @param iv Initial vector for CBC
*/
void aes128_enc(uint8_t* key, uint8_t* data, mode_t mode, uint8_t* iv);

#endif
