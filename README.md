# RDP
Reliable Datagram Protocol

```
    1(6b)         2(1b)     3(2b)   4(2b)    5(2b)
+-----------+-------------+-------+-------+---------+
|           |   |D|R|F|S|A|       |       | Window/ |
| "CSC 361" |000|A|S|I|Y|C| Seq # | Ack # | Payload |
|           |   |T|T|N|N|K|       |       |  Size   |
+-----------+-------------+-------+-------+---------+
```
1. `_magic_` identifies the header as rdp
2. `flags`
3. `Sequence Number`
4. `Acknowledgement Number`
5. `Window/Payload Size`