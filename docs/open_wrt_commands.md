# Open WRT testing commands

```
tc qdisc show
tc qdisc add dev br0 root netem drop 10%
tc qdisc add dev vlan1 root netem drop 10%
tc qdisc del dev br0 root netem drop 10%
tc qdisc del dev clan1 root netem drop 10%
```