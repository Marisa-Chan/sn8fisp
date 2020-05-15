#include "ispsn8f.h"

ISPSN8F chip;

void setup()
{
  chip.Shutdown();

  Serial.begin(57600);
  Serial.print("SN8FMCUISP\n");


  delay(100);
  chip.PowerUp();
  chip.SnMagicWrite();
  delay(100);
}

void result(uint8_t res, uint8_t bytes = 0)
{
  Serial.write(res);
  Serial.write(bytes);
}

int sRead(int wait = 500)
{
  uint32_t stime = millis() + wait;
  while (!Serial.available() )
  {
    if (millis() > stime)
      return -1;
  }
  return Serial.read();
}


void loop() {
  // put your main code here, to run repeatedly:
  int err = 0;
  uint16_t buf[32];
  switch (sRead())
  {
    case 'r':
      {
        if (sRead() != (uint8_t)~'r')
          break;

        int addr = sRead();
        addr |= sRead() << 8;

        int num = sRead();
        if (num > 32)
          num = 32;
        else if (num == 0)
          num = 1;

        if (num == 1)
        {
          if (!chip.SnReadWord(buf, addr))
          {
            err = 255;
            goto ERR;
          }
        }
        else
        {
          if ((addr & 0x1F) + num <= 0x20)
          {
            if (!chip.SnReadNWords(buf, num - 1, addr))
            {
              err = 255;
              goto ERR;
            }
          }
          else
          {
            int rd1 = 0x20 - (addr & 0x1f);
            int rd2 = num - rd1;

            if (!chip.SnReadNWords(buf, rd1 - 1, addr) || !chip.SnReadNWords(&buf[rd1], rd2 - 1, addr + rd1) )
            {
              err = 255;
              goto ERR;
            }
          }
        }

        result(1, num);
        for (int i = 0; i < num; i++)
        {
          Serial.write(buf[i] & 0xff);
          Serial.write((buf[i] >> 8) & 0xff);
        }

        break;
      }
    case 'w':
      {
        if (sRead() != (uint8_t)~'w')
          break;

        int addr = sRead();
        addr |= sRead() << 8;

        int num = sRead();
        if (num > 32 || num < 1)
        {
          err = 255;
          goto ERR;
        }

        for (int i = 0; i < num; i++)
        {
          int a = sRead();
          buf[i] = a | (sRead() << 8);
        }

        if (num == 1)
        {
          if (!chip.SnWriteWord(buf, addr))
          {
            err = 255;
            goto ERR;
          }
        }
        else
        {
          if ((addr & 0x1F) + num <= 0x20)
          {
            if (!chip.SnWriteNWords(buf, num - 1, addr))
            {
              err = 255;
              goto ERR;
            }
          }
          else
          {
            int rd1 = 0x20 - (addr & 0x1f);
            int rd2 = num - rd1;

            if (!chip.SnWriteNWords(buf, rd1 - 1, addr) || !chip.SnWriteNWords(&buf[rd1], rd2 - 1, addr + rd1) )
            {
              err = 255;
              goto ERR;
            }
          }
        }

        result(num, 0);
        break;
      }
      
    case 'e':
      if (sRead() != (uint8_t)~'e')
        break;

      switch (sRead())
      {
        case 0x80:
          {
            int addr = sRead();
            addr |= sRead() << 8;

            if (!chip.SnErasePage(addr))
            {
              err = 255;
              goto ERR;
            }
          }
          break;
        case 0xff:
          if (!chip.SnEraseAll())
          {
            err = 255;
            goto ERR;
          }
          break;
        default:
          err = 255;
          goto ERR;
          break;
      }
      result(1, 0);

      break;

    case 'i':
      if (sRead() != (uint8_t)~'i')
        break;

      switch (sRead())
      {
        case 0:
          if (!chip.SnChipCheck())
          {
            err = 255;
            goto ERR;
          }
          result(1, 0);
          break;
        case 1:
          if (!chip.SnChipID(buf))
          {
            err = 255;
            goto ERR;
          }
          result(1, 2);
          Serial.write(buf[0] & 0xff);
          Serial.write((buf[0] >> 8) & 0xff);
          break;

        default:
          {
            err = 255;
            goto ERR;
          }
          break;
      }
      break;

    case 'p':
      if (sRead() != (uint8_t)~'p')
        break;

      switch (sRead())
      {
        case 0xff:
          chip.Shutdown();
          delay(200);
          chip.PowerUp();
          chip.SnMagicWrite();
          result(1, 0);
          break;

        case 0xe0:
          chip.Shutdown();
          result(1, 0);
          break;
          
        case 0xe1:
          chip.PowerUp();
          result(1, 0);
          break;
          
        case 0xe2:
          chip.SnMagicWrite();
          delay(100);
          result(1, 0);
          break;
          
        default:
          {
            err = 255;
            goto ERR;
          }
          break;
      }
      break;

      break;
    default:
      break;
  }

ERR:

  if (err)
  {
    result(err);
    err = 0;
  }


}
