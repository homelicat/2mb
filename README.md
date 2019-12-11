# Upload
avrdude -c arduino -b 19200 -p m168 -F -P /dev/ttyUSB0 -U flash:w:devctrl.hex
avrdude -c arduino -p m328p -P /dev/ttyACM0 -U flash:w:cpu.hex

# Build
avr-gcc -mmcu=atmega168pa -std=gnu11  -Os devctl.c -o devctl.o
avr-gcc -mmcu=atmega328p -std=gnu11  -Os cpu.c -o cpu.o
avr-objcopy -j .text -j .data -O ihex  devctrl.o  devctl.hex
avr-objcopy -j .text -j .data -O ihex  cpu.o  cpu.hex
