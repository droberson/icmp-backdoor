all:
	gcc -static -o icmp-backdoor icmp-backdoor.c
	strip icmp-backdoor

clean:
	rm -rf icmp-backdoor *~

