all:
	gcc -o rdpr rdpr.c
	gcc -o rdps rdps.c

clean:
	rm *.exe
	rm *.bin
	rm *.o