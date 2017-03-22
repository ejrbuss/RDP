# RDP Reliable Datagram Protocol

1. How do you design and implement your RDP header and header fields?
Do you use any additional header fields?

The header is byte formatted as follows (sizes are in bytes):

```
    1(6b)         2(1b)     3(2b)   4(2b)    5(2b)
+-----------+-------------+-------+-------+---------+
|           |   |D|R|F|S|A|       |       | Window/ |
| "CSC361"  |000|A|S|I|Y|C| Seq # | Ack # | Payload |
|           |   |T|T|N|N|K|       |       |  Size   |
+-----------+-------------+-------+-------+---------+
```
1. `_magic_` identifies the header as rdp
2. `flags` are used to indicate packet types
3. `Sequence Number` indicates sequence number
4. `Acknowledgement Number` indicates acknowledgement number
5. `Window/Payload Size` indiciates window or payload size

Implementation is in `rdp/protocol.c`. The header is
acieved by `memcpy`ing data to a buffer and the inserting
the payload. Parsing is a matter of shifting off the
correct number of bytes also using `memcpy`.

For error control the header will also likely have a checksum added
to it. The parsing function already returns 0 to indicate an invalid header
(when the parsed header does not contain the "CSC361" prefix). The checksum
process will be added hear by recreating the header and running the checksum
on it. Only `rdp/protocol.c` will be concerned with the validity of the
header.

All fields are available through functions after parsing and are dependent
on the last header parsed.

2. How do you design and implement the connection management using SYN,
FIN and RST packets? How to choose the initial sequence number?

Packets types are handled in a sperate process from recieving packets. First
a `listen_rdp` function waits for a timeout or a socket to respond with a
message. Then it attempts to read and parse the message. From here both
the receiver and sender respond to the event (either a received packet or timeout)
depending on their current state. They are implemented roughly like a state
machine from there.

Sequence numbers are slected by random by the receiver when initiating a
connection using rand() truncated to 16 bits.

SYN is sent by the sender to the receiver to initiate a
connection. The receiver uses ACK to respond. The sequence number is incremented
after this exchange.

FIN is sent by the sender to the receiver to end the
connection. The receiver uses ACK to respond. The connection is considered
closed after this exchange. In the final version of this code the receiver
will stay open after sending its ACK to ensure it does no receive another FIN
packet from the sender (indicating its first ACK was dropped).

IF either sender or receiver times out too many times
a RST packet is sent which causes the other connected
party to reset their current action (initiation, data
transfer, or closing). The RST process is not yet fully implemented. Currently
both sender and receiver can handle recieving a RST packet, however neither
send one.

3. How do you design and implement the flow control using window size?
How to choose the initial window size and adjust the size?
How to read and write the file?
How to manage the buffer at the sender and receiver side, respectively?

Currently timeout is set to an arbitrary value and all data is sent at once. In
the final design data will be sent in a process similar to the diagram shown
below.

The file `rdp/filestream` can read an arbitrary number of bytes from an arbitrary
point in the currently opened file. This means the sequence number can be used
to index and the size of the packet can be used to stream. Writing is handled
similarly where any amount of data can be streamed to the file at a time.

Both sender and receiver write (if needed) their data open recieving it.

4. How do you design and implement the error detection, notification and
recovery? How to use timer? How many timers do you use? How to respond to
the events at the sender and receiver side, respectively? How to ensure
reliable data transfer?

Errors caused by corrupted packets will be handled with a checksum in
the final design.

Errors caused by dropped packets will be handled differently depending on the
state:
```
:sender
    :connecting
        ACK
            -> :data-transfer || -> send SYN
        RST
            -> send SYN
        TIMEOUT
            -> send SYN || send -> RST
    :data-transfer
        ACK
            -> move sequence number
               send data equal to window size
        RST
            -> :connecting
        TIMEOUT
            -> resend DATA \\ send -> RST
    :closing
        ACK
            -> close
        RST
            -> send FIN

:receiver
    SYN
        -> send ACK
    RST
        -> wait
    DAT
        -> write data || send -> ACK
    Fin
        -> send ACL
    TIMEOUT
        -> send ACK || send -> RST
```

```
tc qdisc show
tc qdisc add dev br0 root netem drop 10%
tc qdisc add dev vlan1 root netem drop 10%
tc qdisc del dev br0 root netem drop 10%
tc qdisc del dev clan1 root netem drop 10%
```