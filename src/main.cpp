#include <Arduino.h>
#include <BleKeyboard.h>
#include <M5StickC.h>
#include <hwcrypto/aes.h>
#include "mbedtls/aes.h"
#include "SPIFFS.h"

BleKeyboard bleKeyboard("BLEvault","BLEvault",99);

void redrawPinScreen(uint8_t n);
void pinScreen(void);
void checkPin(void);
void printPassword(uint16_t n);
void viewPasswords(void);

uint8_t pin[4]={0,0,0,0};
uint8_t curr_act=0;
uint8_t screen=0;
unsigned char output[32]={};

int n_passwords=0;

mbedtls_aes_context aes;
char key[] = "abcdefghijkl    ";

void setup() {  
  Serial.begin(9600);
  M5.begin();
  M5.IMU.Init();
  M5.Lcd.setRotation(3);
  SPIFFS.begin(false);
  redrawPinScreen(0);
  bleKeyboard.begin();
}


void loop() {
  M5.BtnA.read();
  if(M5.BtnA.pressedFor(20)) {
    //press Big button
    curr_act=1;
    while(M5.BtnA.isPressed()) {
      M5.BtnA.read();
    }
  }

  M5.BtnB.read();
  if(M5.BtnB.pressedFor(20)) {
    //press small button
    curr_act=2;
    while(M5.BtnB.isPressed()) {
      M5.BtnB.read();
    }
  }

  switch(screen) {
    case 0: pinScreen(); break;
    case 1: checkPin(); break;
    case 2: viewPasswords(); break;
  }
  curr_act=0;
}

void redrawPinScreen(uint8_t n) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setCursor(9,15);
  M5.Lcd.setTextSize(6);
  M5.Lcd.setTextFont(1);
  M5.lcd.print(pin[0]);  
  M5.lcd.print(pin[1]);  
  M5.lcd.print(pin[2]);  
  M5.lcd.println(pin[3]);  
  if(n<4) {
    M5.Lcd.setTextColor(TFT_ORANGE);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextFont(1);  
    //21 px from left to center of 1st numb
    //36 px to center of next numb
    M5.Lcd.setCursor(21+(n*36),0);
    M5.Lcd.println("#"); 
  } else {
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextFont(1);  
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.println("CHECK PIN"); 
  }
}

void pinScreen(void) {
  static uint8_t n_dig;
    switch(curr_act) {
    case 1: { 
      if(n_dig<4) {
        if(pin[n_dig]<9) {
          pin[n_dig]++;
        } else {
          pin[n_dig]=0;
        }    
        redrawPinScreen(n_dig);    
      } else {
        screen=1;
      }
    break;
    }
    case 2: {
      if(n_dig<4)
        n_dig++; 
      else 
        n_dig=0; 
      redrawPinScreen(n_dig);
    }
    default: break;
  }
}

void checkPin(void) {
  //copy pin to the rest of key
  for(int i=0; i<4; i++) {
    char buf[2];    
    itoa(pin[i],buf,10);
    key[12+i]=buf[0];
  }

  File file = SPIFFS.open("/passwords", FILE_READ);
  //one line buffer
  char buffer[97];
 // while (file.available()) {
  file.readBytesUntil('\n', buffer, sizeof(buffer));
  //insert end of string
  buffer[96] = 0;

 
  //initialize aes, set aes key
  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_enc( &aes, (const unsigned char*) key, strlen(key) * 8 );

  //convert ascII hex encoded string to char array
  char p[32] = {};
  for(int i=0; i<32; i++) {
    //M5.Lcd.print(buffer[16+(i*2)]);
    //M5.Lcd.print(buffer[16+(i*2)+1]);
    char buf[] = {buffer[16+(i*2)],buffer[16+(i*2)+1], 0x00};
    p[i] = 0xFF & strtol(buf, NULL, 16);
  }
  
  //decrypt string
  unsigned char output[32]={};
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char*)p, output);
  mbedtls_aes_free( &aes );
  file.close(); 
  
  if(output[0]=='O' && output[1]=='K') {
    //get n_passwords
    char pass_buf[4]={buffer[0],buffer[1],buffer[2],0x00};
    n_passwords = strtol(pass_buf, NULL, 10);
    printPassword(1);
    screen=2;
  }else {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.setCursor(9,15);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextFont(1);
    M5.Lcd.println("Wrong PIN!");
    screen=0;
  }
}

void printPassword(uint16_t n) {  
  File file = SPIFFS.open("/passwords", FILE_READ);
  //one line buffer
  char buffer[97];
  //add 1 to omit first record
  for(uint16_t i=0;i<(n+1);i++) {
    file.readBytesUntil('\n', buffer, sizeof(buffer));
  }
  //insert end of string
  buffer[96] = 0;
  
  //initialize aes, set aes key
  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_enc( &aes, (const unsigned char*) key, strlen(key) * 8 );

  //convert ascII hex encoded string to char array
  char p[32] = {};
  for(int i=0; i<32; i++) {
    char buf[] = {buffer[16+(i*2)],buffer[16+(i*2)+1], 0x00};
    p[i] = 0xFF & strtol(buf, NULL, 16);
  }

  //decrypt string  
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char*)p, output);
  mbedtls_aes_free( &aes );
  file.close(); 

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextFont(1);  
  for(uint8_t i=0;i<16;i++) {
    M5.Lcd.printf("%c",buffer[i]);
  }
  M5.Lcd.println(" ");
  M5.Lcd.println(" ");
  for(uint8_t i=0;i<32;i++) {
    M5.Lcd.printf("%c",output[i]);
  }
}

void typePassword(void) {
  if(bleKeyboard.isConnected()) {
  bleKeyboard.releaseAll();
    for(uint8_t i=0; i<16;i++) {      
      if(output[i]==' ')
        break;
      bleKeyboard.press(output[i]&127);
      delay(50);
      bleKeyboard.releaseAll();
    }
  } else {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0,0);
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextFont(1);  
    M5.Lcd.println("Not connected!");
    delay(1000);
  }
}

void viewPasswords(void) {
  static uint16_t n=1;

    switch(curr_act) {
    case 1: { 
      typePassword();
    break;
    }
    case 2: {
      if(n<n_passwords) {
        n++;
        printPassword(n);
      } else {
        n=1;
        printPassword(n);
      }
      break;
    }
    default: break;
  }  
}
