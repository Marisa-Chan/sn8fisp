#!/usr/bin/python3

import serial
import time

class sn8fisp:
	ser = None
	coptaddr = 0
	
	def __init__(self, tty, codeoptaddr = 0x2FFF):
		self.ser = serial.Serial(tty, 57600, timeout=10)
		self.coptaddr = codeoptaddr

		if self.ser.readline().strip() != b"SN8FMCUISP":
			print("Can't open programmer")
			self.ser.close()
			self.ser = None

		print("Found programmer")


	def EnableEraseWrite(self):
		if self.ser == None:
			return False
		return len( self.ReadWords(1, self.coptaddr) ) > 0

	def ReadCodeOptions(self):
		if self.ser == None:
			return False
		return self.ReadWords(1, self.coptaddr)
	
	def ChipCheck(self):
		if self.ser == None:
			return False
		self.ser.write(b"i\x96\x00")
	
		res = self.ser.read(2)
		if (res[0] != 1):
			return False
		
		return True
		
	def ChipID(self):
		if self.ser == None:
			return 0
		self.ser.write(b"i\x96\x01")
	
		res = self.ser.read(2)
		if (res[0] != 1):
			return 0
		
		return int.from_bytes(self.ser.read(2), byteorder="little")

	def ReadWords(self, num, addr):
		if self.ser == None:
			return bytearray()
		bytes = bytearray(b"r\x8d")
		bytes += addr.to_bytes(2, byteorder="little")
		bytes += num.to_bytes(1, byteorder="little")
		self.ser.write(bytes)
	
		res = self.ser.read(2)
		if (res[0] != 1):
			return bytearray()
	
		bytes = bytearray()
		i = 0
		while (i < res[1]):
			bytes += self.ser.read(2)
			i += 1
	
		return bytes

	def WriteWords(self, data, addr):
		if self.ser == None:
			return False
		bytes = bytearray(b"w\x88")
		bytes += addr.to_bytes(2, byteorder="little")
		bytes.append(len(data) // 2)
		self.ser.write(bytes)
		
		self.ser.flush()
		time.sleep(0.01) #wait 
		
		if (len(data) > 32): #Arduino has 64 bytes for serial
			self.ser.write(data[:32])
			self.ser.flush()
			self.ser.write(data[32:])
		else:
			self.ser.write(data)
		
		self.ser.flush()
	
		res = self.ser.read(2)
		if (res[0] != len(data) // 2):
			return False
		return True

	def ErasePage(self, addr):
		if self.ser == None:
			return False
		bytes = bytearray(b"e\x9a\x80")
		bytes += addr.to_bytes(2, byteorder="little")
		self.ser.write(bytes)
	
		oldtimeout = self.ser.timeout
		self.ser.timeout = 60

		res = self.ser.read(2)
		self.ser.timeout = oldtimeout
	
		if (res[0] != 1):
			return False
		return True

	def EraseDevice(self):
		if self.ser == None:
			return False
		self.ser.write(b"e\x9a\xff")
	
		oldtimeout = self.ser.timeout
		self.ser.timeout = 120

		res = self.ser.read(2)
		self.ser.timeout = oldtimeout
	
		if (res[0] != 1):
			return False
		return True

	def ChipReset(self):
		if self.ser == None:
			return False
		self.ser.write(b"p\x8f\xff")
	
		res = self.ser.read(2)
	
		if (res[0] != 1):
			return False
		return True

	def ManualPowerOFF(self):
		if self.ser == None:
			return False
		self.ser.write(b"p\x8f\xe0")
	
		res = self.ser.read(2)
	
		if (res[0] != 1):
			return False
		return True

	def ManualPowerUP(self):
		if self.ser == None:
			return False
		self.ser.write(b"p\x8f\xe1")
	
		res = self.ser.read(2)
	
		if (res[0] != 1):
			return False
		return True
	
	def ManualMagicWrite(self):
		if self.ser == None:
			return False
		self.ser.write(b"p\x8f\xe2")
	
		res = self.ser.read(2)
	
		if (res[0] != 1):
			return False
		return True

