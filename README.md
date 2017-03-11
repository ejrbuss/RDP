# RDP Reliable Datagram Protocol

1. How do you design and implement your RDP header and header fields?
Do you use any additional header fields?

The header is byte formatted as follows (sizes are in bytes):

```
    1(6b)         2(1b)     3(2b)   4(2b)    5(2b)
+-----------+-------------+-------+-------+---------+
|           |   |D|R|F|S|A|       |       | Window/ |
| "CSC 361" |000|A|S|I|Y|C| Seq # | Ack # | Payload |
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
correct number of bytes also using `memcpy`. For error
control the header will also likely have a checksum added
to it.

2. How do you design and implement the connection management using SYN,
FIN and RST packets? How to choose the initial sequence number?

SYN is sent by the sender to the reciever to initiate a
connection. The reciever uses ACK to respond.

FIN is sent by the sender to the reciever to end the
connection. The reciever uses ACK to respond.

IF either sender or reciever times out too many times
a RST packet is sent which causes the other connected
party to reset their current action (initiation, data
transfer, or closing). (Not yet implemented)

3. How do you design and implement the flow control using window size?
How to choose the initial window size and adjust the size?
How to read and write the file?
How to manage the buffer at the sender and receiver side, respectively?

Data is steamed from a file as implemented in
`rdp/filestream.c`.

4. How do you design and implement the error detection, notification and
recovery? How to use timer? How many timers do you use? How to respond to
the events at the sender and receiver side, respectively? How to ensure
reliable data transfer?

Currently only one timer.