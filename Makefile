ARMGNU ?= arm-none-eabi

CFLAGS = -Wall -nostdlib -fomit-frame-pointer -mno-apcs-frame -nostartfiles -ffreestanding -g -march=armv6z -marm -mthumb-interwork -std=gnu11
ASFLAGS = -g -march=armv6z

C_FILES=kernel.c phyAlloc.c hw.c fb.c sched.c syscall.c vmem.c constants.c
AS_FILES=vectors.s

OBJS = $(patsubst %.s,%.o,$(AS_FILES))
OBJS += $(patsubst %.c,%.o,$(C_FILES))
AUDIO=tune.o

.PHONY: gcc clean

gcc : kernel

clean :
	rm -f *.o
	rm -f *.bin
	rm -f *.hex
	rm -f *.elf
	rm -f *.list
	rm -f *.img
	rm -f *.bc
	rm -f *.clang.opt.s
	rm -f *~

tune.o : tune.wav
	$(ARMGNU)-ld -s -r -o $@ -b binary $^

%.o : %.c
	$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@

%.o : %.s
	$(ARMGNU)-as $(ASFLAGS) $< -o $@

kernel : memmap $(OBJS)
	$(ARMGNU)-ld $(OBJS) -T memmap -o kernel.elf
	$(ARMGNU)-objdump -D kernel.elf > kernel.list
	$(ARMGNU)-objcopy kernel.elf -O binary kernel.img
	$(ARMGNU)-objcopy kernel.elf -O ihex kernel.hex

student : clean
	cd .. && ./build_student_archive.sh
