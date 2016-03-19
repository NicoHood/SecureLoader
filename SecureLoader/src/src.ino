#include "AES/aes256_ctr.h"

static uint8_t key[32] = { 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b,
                           0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b,
                           0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
                         };

uint8_t text[32] = "this is my pass to encrypt";

// 3712 1F  33  DC  CB  25  37  E9  12  8D  44  AB  CF  B2  BE  15  EF  
// 3924 1F  33  DC  CB  25  37  E9  12  8D  44  AB  CF  B2  BE  15  EF  

void setup() {
  Serial.begin(115200);
  Serial.println("start");

  // Declare aes256 context variable
  aes256CbcMacCtx_t ctx;

  for (int i = 0; i < AES256_CBC_LENGTH; i++) {
    Serial.print(ctx.cbcMac[i], HEX);
    Serial.print('\t');
  }
  Serial.println();
  Serial.println("---");


  for (int i = 0; i < sizeof(text); i++) {
    Serial.print((char)text[i]);
  }
  Serial.println();
  Serial.println("---");

  // Save key and initialization vector inside context
  aes256CbcMacInit(&ctx, key);

  for (int i = 0; i < AES256_CBC_LENGTH; i++) {
    Serial.print(ctx.cbcMac[i], HEX);
    Serial.print('\t');
  }
  Serial.println();
  Serial.println("---");

  aes256CbcMac(&ctx, text, sizeof(text));

  for (int i = 0; i < AES256_CBC_LENGTH; i++) {
    Serial.print(ctx.cbcMac[i], HEX);
    Serial.print('\t');
  }
  Serial.println();
  Serial.println("---");
  Serial.println("finish");
}

void loop() {
  // put your main code here, to run repeatedly:

}
