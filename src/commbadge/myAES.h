#include <Arduino.h>
#include "mbedtls/aes.h"
#include "mbedtls/md.h"
extern "C" {
#include "crypto/base64.h"
}

void encrypt(char* plainText, char* key, unsigned char* outputBuffer) {
 
  mbedtls_aes_context aes;
 
  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_enc( &aes, (const unsigned char*) key, strlen(key) * 8 );
  mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)plainText, outputBuffer);
  mbedtls_aes_free( &aes );
}
 
void decrypt(unsigned char * chipherText, char * key, unsigned char * outputBuffer){
 
  mbedtls_aes_context aes;
 
  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_dec( &aes, (const unsigned char*) key, strlen(key) * 8 );
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char*)chipherText, outputBuffer);
  mbedtls_aes_free( &aes );
}

void testAES() {
    char* key = "abcdefghijklmnop";

    char* plainText = "Tech tutorials x";
    unsigned char cipherTextOutput[16];
    unsigned char decipheredTextOutput[16];

    encrypt(plainText, key, cipherTextOutput);
    decrypt(cipherTextOutput, key, decipheredTextOutput);

    Serial.println("\nOriginal plain text:");
    Serial.println(plainText);

    Serial.println("\nCiphered text:");
    for (int i = 0; i < 16; i++) {

        char str[3];

        sprintf(str, "%02x", (int)cipherTextOutput[i]);
        Serial.print(str);
    }

    Serial.println("\n\nDeciphered text:");
    for (int i = 0; i < 16; i++) {
        Serial.print((char)decipheredTextOutput[i]);
    }
}

byte * sha256(char* payload, char* key) {
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    const size_t payloadLength = strlen(payload);
    const size_t keyLength = strlen(key);
    byte hmacResult[32];

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)key, keyLength);
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)payload, payloadLength);
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);
    return hmacResult;
}

/*

char * toEncode = "Hello World";
  size_t outputLength;

  unsigned char * encoded = base64_encode((const unsigned char *)toEncode, strlen(toEncode), &outputLength);

  Serial.print("Length of encoded message: ");
  Serial.println(outputLength);

  Serial.printf("%.*s", outputLength, encoded);
  free(encoded);


  char * toDecode = "SGVsbG8gV29ybGQgVGVzdGluZyBiYXNlNjQgZW5jb2Rpbmch";
  size_t outputLength;

  unsigned char * decoded = base64_decode((const unsigned char *)toDecode, strlen(toDecode), &outputLength);

  Serial.print("Length of decoded message: ");
  Serial.println(outputLength);

  Serial.printf("%.*s", outputLength, decoded);
  free(decoded);
*/