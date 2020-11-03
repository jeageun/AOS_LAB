all: loader test test2 testsimple apager dpager hpager

apager : apager.c
	gcc -O0 -w -g -static apager.c -o apager.o -Wl,--script=loaders.lds

dpager : dpager.c
	gcc -O0 -w -g -static dpager.c -o dpager.o -Wl,--script=loaders.lds

hpager : hpager.c
	gcc -O0 -w -g -static hpager.c -o hpager.o -Wl,--script=loaders.lds

loader : loader.c
	#gcc -O0 -w -g -static -static-libgcc -Wl,-Ttext-segment=0x20000000 -o loader.o loader.c
	gcc -O0 -w -g -r -static -static-libgcc loader.c -o loader.o
	ld -T loaders.lds -o loader.exe loader.o

test : test1.asm
	nasm -w+all -f elf64 -o 'test1.o' 'test1.asm'
	ld -T script.lds -o test1.exe test1.o


test2 : hello.c
	gcc -O0 -w -g -r  -static -static-libgcc hello.c -o hello.o
	ld -T script.lds -o hello.exe hello.o
	#gcc -O0 -w -g -static -static-libgcc -Wl,-Ttext-segment=0x4000000 -o hello.o hello.c
	#ld -o hello.exe hello.o

testsimple : testsimple.c
	gcc -O0 -w -r -g -static -o simple.o testsimple.c
	ld -T script.lds -o simple.exe simple.o
	#ld -o simple.exe simple.o

clean:
	rm *.o *.exe
