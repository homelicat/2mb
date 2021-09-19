# 2mb
**Twórca:** lolikotek

**Jezyk:** C

**Data:** druga polowa 2019

**Opis:**

Projekt sklada się z komputera i programu. Komputer jest zrobiony na dwuch mikrokontrolerach: stm8s103f3 jak kontroler urządzen i atmega328p jak processor centralny. Oni kommunikują się między sobą za pomocą mojego protokołu, nadbudowy dla UART. Pisalem na C z bibliotekami standartowymi. Dla tego komputera zrobilem prosty interpretowany język z okolo assemblerną składnią.

# Upload
    stm8flash -c stlinkv2 -p stm8s103f3 -w devctrl.ihx
    avrdude -c arduino -p m328p -P /dev/ttyACM0 -U flash:w:cpu.hex

# Build
    sdcc --Werror --std-sdcc99 -mstm8 -DSTM8S103 -lstm8 -mstm8 --out-fmt-ihx ../devctrl.c
    avr-gcc -mmcu=atmega328p -std=gnu11  -Os cpu.c -o cpu.o
    avr-objcopy -j .text -j .data -O ihex  cpu.o  cpu.hex
