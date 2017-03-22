all:
	gcc -o rdpr rdpr.c
	gcc -o rdps rdps.c

clean:
	rm receive.txt
	rm *.exe
	rm *.bin
	rm *.o