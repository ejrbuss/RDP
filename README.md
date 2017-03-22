# RDP
## Reliable Datagram Protocol

Eric Buss V00818492 B03

**NOTE: if you are viewing this file in raw markdown, it is recommended
you view it as a pdf in `docs/README.pdf`**

## Table of Contents
 1. Introduction
 2. Running the Demo
 3. Configuration
 3. Design
    1. Header
    2. Control Flow
    3. Flow Control
    4. Error Control
    5. Statistics
 4. API
    1. Creating a Packet
    2. Opening a File
    3. Creating a Sender
    4. Creating a Receiver
 5. Contact & Credits

 ## Introduction

RDP is a UDP based protocol that provides connection management, flow
control, and error control. RDP provides a robust means of sending data
across lossy networks. This document details for how to get a demo of RDP
running (a simple file transfer application), the RDP design, and the API.

 ## Running the Demo

To run RDP you will need to compile it from source with a C compiler that
supports (eg. GCC):

 - `stdint.h`
 - `sys/socket.h`
 - `sys/types.h>`
 - `netinet/in.h>`
 - `unistd.h`
 - `arpa/inet.h`
 - `time.h`

To compile the source for the demo run
```bash
$ make # or
$ make -B
```

The demo is composed of two binaries `rdpr` (the receiver) and `rdps`
(the sender). These two files allow you to transfer a file over a lossy
netowrk. You will need to run `rdpr` first to wait for the connection.
`rdpr` expects the following command line arguments
```bash
rdpr [options] <IP> <port> <file>
```

Where `IP:port` is the socket to listen on and `file` is the write destination. Optionally you can run `rdpr` with the `-t` command to run it
with preset arguments.

Once `rdpr` is running you can run `rdps` in a sepearte terminal. `rdps`
expects the following command line arguments
```bash
rdps [options] <IP> <port> <receiver IP> <receiver port> <file>
```

Where `IP:port` is the socket to listen on and `receiver IP:receiver port`
is the socket to send to. `file` is the file to transfer. Optionally you
can run `rdps` with the `-t` command to run it with preset arguments.

Both `rdpr` and `rdps` log sent and recieved packets. For further details
check `docs/requirements.pdf` in the root directory of the repository. You
can also run either binary with the `-h` or `--help` command to get a full
list of options.

## Configuration

Performance of RDP can be modified by changing a number of constants in
`rdp/net_config.h`. These are the relevant default values
```
#define TIMEOUT          8
#define MAXIMUM_TIMEOUTS 256
#define WINDOW_SIZE      16
```

These values were selected to provide a reasonable transfer speed with a
wide range of network speeds. For high quality netorks its recommended to
raise the `WINDOW_SIZE` to `64` or even `128`. Note that for `128` you
should also raise the `TIMEOUT` for most networks. If your network has more
than a 10% drop rate it is recommended you raise `MAXIMUM_TIMEOUTS`.

## Design
### Header

RDP is managed using a small 15 byte header. The header is used to carry
meta data about each packet in order to coordinate data transfer. The
header is implemented in `rdp/protocol.c`. Below shows the encoding
of the header with each section and the number of bytes allocated to each.
```
     1(6b)       2(1b)       3(4b)      4(2b)      5(2b)
+----------+--------------+-----------+---------+----------+
|          |  |R|R|D|F|A|S|           | Window/ |          |
| "CSC361" |00|E|S|A|I|C|Y| Seq/Ack # | Payload | Checksum |
|          |  |S|T|T|N|K|N|           |   Size  |          |
+----------+--------------+-----------+---------+----------+
```
The table below describes each field.

|   | Section | Description | Bytes |
| - | ------- | ------------ | ----- |
| 1 | Prefix | The bytes "CSC361" indicating this is an RDP packet | 6 |
| 2 | Flags | Flags indicating the packet type | 1 |
| 3 | Seq/Ack Number | Seq number for Sender, Ack for Receiver | 4 |
| 4 | Payload/Window size | Payload number for Sender, Window for receiver | 2 |
| 5 | Checksum | Checksum for header and payload | 2

The use of each section is detailed in the following sections.

### Control Flow

RDP's base packet flow functions very similarly to TCP. A connection is
started when a RDP receiver recieves a packet with the `SYN` flag set
(In the future packets with a particular flag set will be reffered to as
a `FLAG` packet eg. a `SYN` packet). The RDP receiver will now begin
repeatedly sending `ACK` packets on a timeout indicating it recieved the
`SYN` packet.

Once the `SYN` packet receiver has recieved an `ACK` they can send `DAT
`packets up to and including their last recieved `ACK`s window size. These
packets should be sent in order and be filled to the max packet size. The
sender should then wait for an `ACK` packet and then repeat this proccess.

Once the sender is done sending `DAT` packets they send `FIN` packets until
they recieve and `ACK` packet in response. They can immediately close the
connection once they recieve the appropriate `ACK` packet. The receiver
on the otherhand must continue to wait and respond to `FIN` packets until
they timeout.

Ideally this process is ideally described by the diagram below. Note that
the number of `DAT` packets sent by the sender on the left corresponds to
the default `WINDOW_SIZE` set in `rdp/net_config.h`.

<img src="docs/rdp_ideal.png">

*Ideal packet flow*

On a poor network packets can be dropped at any part of this process.
RDP handles each phase (connecting, data transfer, and disconnecting)
differently. During connection the sender will continue to send `SYN`
packets until they recieve a `ACK` packet with an ack number equal to
1 + the expected sequence number. The receiver will continue to send `ACK`
packets from this point onwards until they recieve a `FIN` packet during
the disconnecting phase. This process is illustrated in the diagram below.

<img src="docs/rdp_non_ideal_connect.png">

*Non ideal connecting packet flow*

During data transfer RDP has to deal with dropped and out of order packets.
RDP's window size corresponds to a buffer that supports out of order
packets (OOO buffer). When the receiver receives a `DAT` packet with a
sequence number it is not expecting (ie. the next byte it hopes to write) it
places that packet in the OOO buffer. Once this buffer is full packets will
be dropped. Packets with sequence numbers less than the expected sequence
number are also dropped. When a `DAT` packet with the desired sequence
number arrives the OOO buffer is emptied of all `DAT` packets that can be
streamed to the file. While there are items in the buffer the reciever sends
`ACK` packets with their desired sequence number and a window size of
OOO buffer capacity (ie. Max Window size - # of out of order packets). This
process is illustrated in the diagram below.

<img src="docs/rdp_non_ideal_data_transfer.png">

*Non ideal data transfer packet flow*

During disconnection it is up to the reciever to close the connection
responsibly...

<img src="docs/rdp_non_ideal_disconnect.png">

*Non ideal disconnecting packet flow*

### Error Control

Outside of out of order packets (whose handling is described prior in the
section on Control flow), corrupted packets are handled via a checksum.
The checksum is a standard inversion of the sum of 16 bit words of the
packet. This sum is run on the entire packet minus the checksum. The packet
is then reconstructed on the other side with everything except the checksum
and then recomputed. The checksum is run on the payload as well as the
header is relatively small and a checksum on it alone would increase the
chances of a collision.

### Statistics

...

## API

The following is a short description of the RDP API. It is not at all
exhaustive or detailed. For a better description please read the source.

### Creating a Packet

Packets can be created using `rdp_pack()` function. Note that flags must be
but encoded at this point and all integers are expected to be unsigned.
```
rdp_pack(buffer, flags, seq number, size, payload);
```

Buffer will now containt the packet. Buffer is expected to be of
at least size `rdp_MAX_PACKET_SIZE`. If you need the size of a packet you
can get it using
```
rdp_packed_size(size);
```

Where size is the payload size. If you have recieved a packet and wish to
parse it you can call
```
rdp_parse(packet);
```

At this point `rdp_flags()`, `rdp_seq_ack_number()`, `rdp_window_size()`,
`rdp_payload_size()`, `rdp_payload()` will all return relevant information
to the last parsed packet. If the packet was corrupted or is not a valid
RDP packet `rdp_parse()` will return 0.

### Opening a File

To open a file for reading
```
rdp_filestream_open(filename, 'r');
```

To open a file for writing
```
rdp_filestream_open(filename, 'w');
```

To close any files
```
rdp_filestream_close();
```

### Creating a Sender

To create a sender use `rdp_sender()`
```
rdp_sender(source IP, source port, destiantion UP, destination port);
```

Each phase can be called with `rdp_sender_connect()`, `rdp_sender_send()`,
and `rdp_sender_disconnect()`. For live stats you can call
`rdp_sender_stats()` or `rdp_stats()`. The first will print the sender stats
the second will provide an array of stats.

### Creating a Receiver

To create a sender use `rdp_reciever()`
```
rdp_reciever(source IP, source port);
```

The receiver can than be set to listen using `rdp_reciever_receive()`. For live stats you can call `rdp_receiver_stats()` or `rdp_stats()`. The first
will print the sender stats the second will provide an array of stats.

## Contact & Credits

My name is Eric Buss. This project was done for the University of
Victoria's CSC 361. If you have questions about this or other projects
you can reach me at `ejrbuss@gmail.com`.

```
tc qdisc show
tc qdisc add dev br0 root netem drop 10%
tc qdisc add dev vlan1 root netem drop 10%
tc qdisc del dev br0 root netem drop 10%
tc qdisc del dev clan1 root netem drop 10%
```