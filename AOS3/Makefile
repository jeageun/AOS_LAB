all: test2 testsimple apager dpager hpager testgemm testtouch

apager : apager.c
	gcc -O0 -w -g -static apager.c -o apager.o -Wl,--script=loaders.lds

dpager : dpager.c
	gcc -O0 -w -g -static dpager.c -o dpager.o -Wl,--script=loaders.lds

hpager : hpager.c
	gcc -O0 -w -g -static hpager.c -o hpager.o -Wl,--script=loaders.lds

test2 : hello.c
	gcc -O0 -w -g -r  -static -static-libgcc hello.c -o hello.o
	ld -T script.lds -o hello.exe hello.o

testtouch : touchsome.c
	gcc -O0 -w -r -g -static -o touch.o touchsome.c
	ld -T script.lds -o touch.exe touch.o

testsimple : testsimple.c
	gcc -O0 -w -r -g -static -o simple.o testsimple.c
	ld -T script.lds -o simple.exe simple.o

testgemm : gemm.c
	gcc -O0 -w -r -g -static -o gemm.o gemm.c
	ld -T script.lds -o gemm.exe gemm.o

clean:
	rm *.o *.exe
