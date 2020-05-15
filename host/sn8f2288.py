import argparse
import sn8fisp
import time

parser = argparse.ArgumentParser(description='SN8F2288 Programmer')

parser.add_argument('tty', metavar='ttyUSB')
parser.add_argument('-r', '--read', metavar='file')
parser.add_argument('-w', '--write', metavar='file')

args = parser.parse_args()

isp = sn8fisp.sn8fisp(args.tty)

if isp.ser == None:
	exit(-1)

if not isp.ChipCheck():
	print("No chip found")
	exit(-2)

chipID = isp.ChipID()

if (chipID & 0xFFF0) != 0x2700:
	print("Wrong chip")
	exit(-2)

print("Chip ID:", hex(chipID))

if args.read != None:
	print("Reading into:", args.read)
	fl = open(args.read, "wb")
	if fl != None:
		if not isp.EnableEraseWrite():
			print("Can't disable secure")
			exit(-3)
		i = 0
		while i < 0x3000:
			bytes = isp.ReadWords(32, i)
			if len(bytes) != 64:
				print("Read error")
				exit(-3)
			
			if i % ((0x3000 // 32) // 100 * 32) == 0:
				print(i * 100 // 0x3000, "%")
			
			fl.write(bytes)
			i += 32
		print("Reading complete")
		fl.close()
	else:
		print("Can't open file", args.read)
		exit(-4)

if args.write != None:
	print("Writing file:", args.write)
	fl = open(args.write, "rb")
	if fl != None:
		bts = fl.read()
		if len(bts) != 0x3000 * 2:
			print("File size is not 0x3000 * 2")
			exit(-4)
		
		if not isp.EnableEraseWrite():
			print("Can't enable erase/write")
			exit(-3)

		#print(private.hex())
		
		print("Erasing device")
		if not isp.EraseDevice():
			print("Erase failed")
			exit(-3)
		
		if not isp.EnableEraseWrite():
			print("Can't enable erase/write")
			exit(-3)
		
		private = isp.ReadWords(12, 0x3000)
		
		if (len(private) != 24):
			print("Can't read private data")
			exit(-3)
		
		pOpt = private[4 * 2:5 * 2]
		Opt = bts[0x2FFF * 2 : 0x3000 * 2]
		
		if (pOpt != Opt):
			print("Changing code option in private", pOpt.hex(), "->", Opt.hex())
			private = private[:4*2] + Opt + private[5*2:]
			print("Writing new private bytes:", private.hex())
			
			if not isp.ErasePage(0x3000):
				print("Can't erase private")
				exit(-3)
			if not isp.WriteWords(private, 0x3000):
				print("Can't write private")
				exit(-3)
		
			
		print("Writing")
		i = 0
		while i < 0x3000:
			if not isp.WriteWords(bts[0:64], i):
				print("Write error")
				exit(-3)
			
			bts = bts[64:]
			
			if i % ((0x3000 // 32) // 100 * 32) == 0:
				print(i * 100 // 0x3000, "%")
			
			i += 32
		print("Writing complete")
		fl.close()
	else:
		print("Can't open file", args.write)
		exit(-4)

