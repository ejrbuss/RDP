rdp: rdpr.c rdps.c rdp/filestream.c rdp/netconfig.c rdp/receiver.c rdp/sender.c rdp/protocol.c rdp/util.c
	gcc -o rdpr rdpr.c rdp/filestream.c rdp/netconfig.c rdp/receiver.c rdp/protocol.c rdp/util.c
	gcc -o rdps rdps.c rdp/filestream.c rdp/netconfig.c rdp/sender.c rdp/protocol.c rdp/util.c

clean:
	rm *.o rdpr rdps rdpr.exe rdps.exe