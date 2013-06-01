all: botmind.hex

botmind.hex: botmind.o
	sdcc -mpic14 -p16f628a -V --debug -Wl-c -Wl-m -Lnonfree -obotmind.hex botmind.o

botmind.o: botmind.asm
	gpasm -c -I. -w0 botmind.asm

botmind.asm: botmind.c
	sdcc -mpic14 -p16f628a -V --debug -Inonfree -c botmind.c

# Use picprog to read the microcontroller.
read:
	picprog --output readbotmind.hex --burn -d pic16f628a

# Use picprog to write the microcontroller.
write: botmind.hex
	picprog --input botmind.hex --burn -d pic16f628a 

# Use picprog to erase the microcontroller. 
erase:
	picprog --erase --burn -d pic16f628a 

clean:
	rm -rf botmind.adb botmind.asm botmind.cod botmind.cof botmind.hex botmind.lst botmind.map botmind.o botmind.p readbotmind.hex
