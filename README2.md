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
    4. Creating a Reciever
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

Where `IP:port` is the socket to listen on and `reciever IP:reciever port`
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
| 4 | Payload/Window size | Payload number for Sender, Window for Reciever | 2 |
| 5 | Checksum | Checksum for header and payload | 2

The use of each section is detailed in the following sections.

### Control Flow

RDP's base packet flow functions very similarly to TCP. A connection is
started when a RDP reciever recieves a packet with the `SYN` flag set
(In the future packets with a particular flag set will be reffered to as
a `FLAG` packet eg. a `SYN` packet). The RDP reciever will now begin
repeatedly sending `ACK` packets on a timeout indicating it recieved the
`SYN` packet.

Once the `SYN` packet reciever has recieved an `ACK` they can send `DAT
`packets up to and including their last recieved `ACK`s window size. These
packets should be sent in order and be filled to the max packet size. The
sender should then wait for an `ACK` packet and then repeat this proccess.

Once the sender is done sending `DAT` packets.

<img src="docs/rdp_ideal.png">

*Ideal packet flow*

<img src="docs/rdp_non_ideal_connect.png">

*Non ideal connect packet flow*

<img src="docs/rdp_non_ideal_data_transfer.png">

*Non ideal data transfer packet flow*

<img src="docs/rdp_non_ideal_disconnect.png">

*Non ideal disconnect packet flow*

### Flow Control
### Error Control
### Statistics
## API
### Header
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