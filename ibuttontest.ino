  #include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "OneWire.h"

#include "ds1961.h"
#include "editline.c"
#include "hexutil.c"

static OneWire ow(2);
static DS1961 ds(&ow);

void hexdump(uint8_t *data, int size, int modulo)
{
    int i, len;
    int addr = 0;
    uint8_t b;

    while (size > 0) {
        if ((modulo > 0) && (size > modulo)) {
            len = modulo;
        } else {
            len = size;
        }
        size -= len;

        for (i = 0; i < len; i++) {
            b = data[addr++];
            Serial.print((b >> 4) & 0xF, HEX);
            Serial.print((b >> 0) & 0xF, HEX);
        }
    }
}

static bool DoPoll(uint8_t id[8])
{  
  // identify
  ow.reset_search();
  if (!ow.search(id)) {
    return false;
  }

  Serial.print("<");
  hexdump(id, 8, 0);
  Serial.print(">");
  
  return true;
}

static bool DoReadAuthWithChallenge(const uint8_t id[], const char *str)
{
  uint8_t challenge[] = {0, 0, 0};
  uint8_t data[32];
  uint8_t mac[20];
    
  if (!parseHexString(str + 2, challenge, sizeof(challenge))) {
//    Serial.println("E Parse challenge failed!");
    return false;
  }
    
  if (!ds.ReadAuthWithChallenge(id, 0, challenge, data, mac)) {
//    Serial.println("E ReadAuthWithChallenge failed!");
    return false;
  }
  
  hexdump(data, 32, 0);
  Serial.print(" ");
  hexdump(mac, 20, 0);
  
  return true;
}

static bool DoWriteSecret(const uint8_t id[], const char *str)
{
  uint8_t secret[8];
  
  if (!parseHexString(str + 2, secret, sizeof(secret))) {
//    Serial.println("E Parse secret failed!");
    return false;
  }
  
  if (!ds.WriteSecret(id, secret)) {
//    Serial.println("E ReadAuthWithChallenge failed!");
    return false;
  }
  
  return true;
}

static bool DoWriteData(const uint8_t id[], const char *str)
{
  int addr;
  uint8_t data[8];
  uint8_t mac[20];
  
  if (!parseHexString(str + 2, data, sizeof(data))) {
    Serial.println("E Parse data failed!");
    return false;
  }
  
  if (!parseHexString(str + 19, mac, sizeof(mac))) {
    Serial.println("E Parse mac failed!");
    return false;
  }
  
  if (!ds.WriteData(id, addr, data, mac)) {
    Serial.println("E DS1961WriteData failed!");
    return false;
  }

  return true;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("RESET");
}

void loop()
{
  static uint8_t id[8];
  static char line[30];
  bool haveLine;
  bool ret;
  char c;
  
  // process incoming serial chars
  haveLine = false;
  while (Serial.available() && !ret) {
    haveLine = EditLine(Serial.read(), &c, line, sizeof(line));
    Serial.print(c);
  }
  
  // process line
  if (haveLine) {
    Serial.print(line[0]);
    Serial.print(" ");
    switch (line[0]) {
    case 'P':
      ret = DoPoll(id);
      break;
    case 'C':
      ret = DoReadAuthWithChallenge(id, line);
      break;
    case 'S':
      ret = DoWriteSecret(id, line);
      break;
    case 'W':
      ret = DoWriteData(id, line);
      break;
    case '\0':
      // ignore empty line
      break;
    default:
      ret = false;
      break;
    }
    Serial.print(" ");
    Serial.println(ret ? "OK" : "ERROR");
  }
}
