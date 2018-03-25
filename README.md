# clockd
## About
This is a simple time synchronization solution between hosts on a LAN.  It
implements RFC 868 on the TCP side.
## Usage
The client and server live in the same binary.
```console
./clockd -[Sc:s]
	-S 	allow client mode to set the clock
	-c 	run in client mode
	-s	run in server mode (must run as root)
```

