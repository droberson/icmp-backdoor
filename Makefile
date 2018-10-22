all:
	gcc -static -o icmp-backdoor icmp-backdoor.c

clean:
	rm -rf icmp-backdoor *~

