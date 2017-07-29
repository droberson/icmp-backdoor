all:
	gcc -o icmp-backdoor icmp-backdoor.c

clean:
	rm -rf icmp-backdoor *~

