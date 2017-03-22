all:
	gcc -o rdpr rdpr.c
	gcc -o rdps rdps.c
	rm receive.txt

clean:
	rm *.exe
	rm *.bin
	rm *.o