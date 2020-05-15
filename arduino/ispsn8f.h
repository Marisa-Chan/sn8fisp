#ifndef SN8F_H_
#define SN8F_H_

#include "Arduino.h"

#define POWEROFFTIME   100
#define OPDELAY        100

class ISPSN8F
{
  protected:
    enum
    {
      MODE_UNK = 0,
      MODE_WRITE = 1,
      MODE_READ = 2,
      MODE_WRDATA = 3,

      RES_OK = 0xEA,
    };

    enum
    {
      OP_CHECK      = 0,
      OP_ERASE_PAGE = 0x40, // 128 words
      OP_CHIPID     = 0x4A,
      OP_ERASE_ALL  = 0x60,
      OP_READ       = 0x80, // READ  1 WORD ( 0x80 - 0x9F : 1 - 32 word read )
      OP_WRITE      = 0xC0, // WRITE 1 WORD ( 0xC0 - 0xDF : 1 - 32 word write )
    };
  public:
    bool SnChipCheck()
    {
      return OpWrite(OP_CHECK);
    }

    bool SnReadWord(uint16_t *out, uint16_t addr)
    {
      if (!out || !SnChipCheck() || !OpWrite(OP_READ) || !AddressWrite(addr))
        return false;

      Clk();
      Clk();
      uint16_t res = ReadResultWordLSB();

      if ( !ReadResultMSB() )
        return false;

      Clk();
      Clk();

      *out = res;
      return true;
    }
    
    // Read 1+N words
    // Wraps every 0x20 address.
    // Example: read 32 from 0xFF0 - it will read 0xFF0-0x1000, wraps and read 0xFE0-0xFF0
    bool SnReadNWords(uint16_t *out, int num, uint16_t addr)
    {
      num &= 0x1F;
      if (!out || !SnChipCheck() || !OpWrite(OP_READ + num) || !AddressWrite(addr))
        return false;

      Clk();
      Clk();
      for (int i = 0; i <= num; i++)
        out[i] = ReadResultWordLSB();

      if ( !ReadResultMSB() )
        return false;

      Clk();
      Clk();

      return true;
    }

    bool SnWriteWord(uint16_t *in, uint16_t addr)
    {
      if (!in || !SnChipCheck() || !OpWrite(OP_WRITE) || !AddressWrite(addr) || !WordMSBWrite(*in))
        return false;

      if ( !ClkWaitResultMSB(0x800) )
        return false;

      Clk();
      Clk();
      return true;
    }

    // Write 1+N words
    // Wraps every 0x20 address.
    // Example: write 32 from 0xFF0 - it will write 0xFF0-0x1000, wraps and write 0xFE0-0xFF0
    bool SnWriteNWords(uint16_t *in, int num, uint16_t addr)
    {
      num &= 0x1F;
      if (!in || !SnChipCheck() || !OpWrite(OP_WRITE + num) || !AddressWrite(addr))
        return false;

      for (int i = 0; i <= num; i++)
      {
        if ( !WordMSBWrite( in[i] ) )
          return false;
      }

      if ( !ClkWaitResultMSB(0x800) )
        return false;

      Clk();
      Clk();
      return true;
    }

    bool SnEraseAll()
    {
      if ( !SnChipCheck() || !OpWrite(OP_ERASE_ALL) || !AddressWrite(0) || !ClkWaitResultMSB(0x60000) )
        return false;
      Clk();
      Clk();
      return true;
    }

    bool SnErasePage(uint16_t page)
    {
      page &= ~0x7F;
      
      if ( !SnChipCheck() || !OpWrite(OP_ERASE_PAGE) || !AddressWrite(page) || !ClkWaitResultMSB(0x9600) )
        return false;
      Clk();
      Clk();
      return true;
    }
    
    bool SnChipID(uint16_t *out)
    {
	if (!out || !SnChipCheck() || !OpWrite(OP_CHIPID) )
	  return false;
	
	Clk();
	Clk();
	Clk();
	uint16_t res = ReadResultWordLSB();

        if ( !ReadResultMSB() )
          return false;

        Clk();
        Clk();

        *out = res;
        return true;
    }

    void SnMagicWrite()
    {
      MagicWrite(0x6A, 0x69);
      MagicWrite(0x95, 0x96);
      MagicWrite(0x95, 0x96);
      MagicWrite(0x6A, 0x69);
      MagicWrite(0x95, 0x96);
      MagicWrite(0x6A, 0x69);
      MagicWrite(0x6A, 0x69);
      MagicWrite(0x94, 0x96);
    }

  public:
    ISPSN8F(uint16_t cSz = 0x3000, uint16_t uSz = 0x2FF8,
         int pow = 8, int clk = 9,
         int oe = 10, int pgm = 11,
         int pdb = 12)
      : _pPDB(pdb), _pPGM(pgm)
      , _pOE(oe),   _pCLK(clk)
      , _pPOW(pow)
    {}

    void PowerUp()
    {
      Shutdown();

      delay(POWEROFFTIME);

      digitalWrite(_pPOW, LOW); // power up
    }

    void Shutdown()
    {
      pinMode(_pPOW, OUTPUT);
      pinMode(_pCLK, OUTPUT);
      pinMode(_pOE,  OUTPUT);
      pinMode(_pPGM, OUTPUT);
      pinMode(_pPDB, OUTPUT);

      // No power from any pins
      digitalWrite(_pCLK, LOW);
      digitalWrite(_pOE,  LOW);
      digitalWrite(_pPGM, LOW);
      digitalWrite(_pPDB, LOW);
      digitalWrite(_pPOW, HIGH); // power down

      _mode = MODE_UNK;
    }

    

    void SetWriteMode()
    {
      if (_mode != MODE_WRITE)
      {
        pinMode(_pPDB, OUTPUT); // Read pin to output
        digitalWrite(_pPDB, LOW); //and low

        digitalWrite(_pCLK, LOW);
        digitalWrite(_pOE,  LOW);

        digitalWrite(_pPGM, HIGH); // We will write
        delayMicroseconds(OPDELAY);

        _mode = MODE_WRITE;
      }
    }

    void SetReadMode()
    {
      if (_mode != MODE_READ)
      {
        pinMode(_pPDB, INPUT); // Read pin to input
        digitalWrite(_pPDB, LOW); //pull-up enable is needed? (if so - do HIGH)

        digitalWrite(_pCLK, LOW);
        digitalWrite(_pOE,  LOW);

        digitalWrite(_pPGM, LOW); // We will read
        delayMicroseconds(OPDELAY);

        _mode = MODE_READ;
      }
    }

    void Clk()
    {
      digitalWrite(_pCLK, HIGH);
      delayMicroseconds(OPDELAY);
      digitalWrite(_pCLK, LOW);
      delayMicroseconds(OPDELAY);
    }

    void MagicWrite(uint8_t p, uint8_t o)
    {
      for (int i = 0; i < 8; i++)
      {
        digitalWrite(_pPGM, ((p & 0x80) != 0 ? HIGH : LOW) );
        digitalWrite(_pOE, ((o & 0x80) != 0 ? HIGH : LOW) );
        delayMicroseconds(OPDELAY);
        p <<= 1;
        o <<= 1;
        Clk();
      }
    }

    bool ReadResultMSB()
    {
      SetReadMode();

      uint8_t db = 0;
      for (int i = 0; i < 8; i++)
      {
        Clk();
        db <<= 1;
        db |= digitalRead(_pPDB);
      }
      Clk();

      return db == RES_OK;
    }

    bool OpWrite(uint8_t op)
    {
      SetWriteMode();

      uint8_t nop = ~op;

      for (int i = 0; i < 8; i++)
      {
        digitalWrite(_pOE, ((op & 0x80) != 0 ? HIGH : LOW) );
        op <<= 1;
        Clk();
      }

      for (int i = 0; i < 8; i++)
      {
        digitalWrite(_pOE, ((nop & 0x80) != 0 ? HIGH : LOW) );
        nop <<= 1;
        Clk();
      }

      return ReadResultMSB();
    }

    bool WordMSBWrite(uint16_t addr)
    {
      SetWriteMode();

      uint16_t naddr = ~addr;

      digitalWrite(_pPGM, LOW);
      _mode = MODE_WRDATA;

      for (int i = 0; i < 16; i++)
      {
        digitalWrite(_pOE, ((addr & 0x8000) != 0 ? HIGH : LOW) );
        addr <<= 1;
        Clk();
      }

      for (int i = 0; i < 16; i++)
      {
        digitalWrite(_pOE, ((naddr & 0x8000) != 0 ? HIGH : LOW) );
        naddr <<= 1;
        Clk();
      }

      return ReadResultMSB();
    }

    bool AddressWrite(uint16_t addr)
    {
      return WordMSBWrite(addr);
    }

    bool ClkWaitResultMSB(uint32_t cmax)
    {
      SetReadMode();

      for (uint32_t i = 0; i < cmax; i++)
      {
        Clk();
        if ( digitalRead(_pPDB) )
          break;
      }

      if (!digitalRead(_pPDB))
        return false;

      uint8_t res = 0;
      for (int i = 0; i < 8; i++)
      {
        res <<= 1;
        if (digitalRead(_pPDB))
          res |= 1;
        Clk();
      }

      return res == RES_OK;
    }

    uint16_t ReadResultWordLSB()
    {
      SetReadMode();
      uint16_t db = 0;
      for (int i = 0; i < 16; i++)
      {
        Clk();
        db >>= 1;
        db |= (digitalRead(_pPDB) == HIGH ? 0x8000 : 0);
      }
      return db;
    }

  protected:
    const int _pPDB = 12;
    const int _pPGM = 11;
    const int _pOE  = 10;
    const int _pCLK = 9;
    const int _pPOW = 8;

    int _mode = MODE_UNK;
};




#endif // SN8F_H_
