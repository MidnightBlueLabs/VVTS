# `trafficmonitor`

The `trafficmonitor` class provides traffic monitoring and attack validation primitives. It is instantiated as follows:
```
mon = trafficmonitor();
```

## Methods

### `detect_vpn(state_vpn_detected, state_err)`
Wait until VPN traffic is detected flowing to/from the VPN endpoints set in the `vpn_endpoints` property. The state machine enters state `state_vpn_detected` when such traffic is detected, and `state_err` in case of an error.
`vpn_endpoints` (a string property describing the endpoints) and `interface` (the virtual network interface pertaining to the access point) must be set prior to invoking `detect_vpn`.

### `detect_attack(state_safe, state_vulnerable, state_err)`
Wait until the validation server set in the `validation_server` property indicates that the client has reached out. If so, the VPN client is deemed vulnerable if traffic flowing directly to the validation server was detected. If only traffic flowing to (one of) the VPN endpoints set in the `vpn_endpoints` property is detected, the VPN client is deemed safe. If neither direct nor encapsulated traffic was detected, the test is deemed inconclusive and this is considered an error. Subsequently, the state machine enters state `state_safe`, `state_vulnerable` or `state_err` accordingly.
`vpn_endpoints` (a string property describing the endpoints), `interface` (the virtual network interface pertaining to the access point) and `validation_server` (the URL of the server component) must be set prior to invoking `detect_attack`.

## Writable properties

### `interface`
A string containing the virtual network interface of our access point, which should be obtained by reading the `ap_interface` property of an instance of the `accesspoint` class.

### `vpn_endpoints`
A string containing descriptors of the various VPN endpoints, joined together with semicolons (`;`). A VPN endpoint descriptor is of the following format:
```
[proto]://[hostname]:[port]
```
Where `proto` is either `tcp` or `udp`. Hostname can be either an IPv4 or IPv6 address, or a domain name that is subsequently resolved.

Alternatively, the following format is also valid
```
[proto]://[hostname]
```
Where `proto` is either `gre`, `ipsec` or `l2tp`. As these protocols do not have a concept of a port, no port number is to be provided.

### `validation_server`
A string containing the URL of the server component, used to distinguish between the situation where traffic flows through the VPN (and therefore unaffected by the attack in progress), and where traffic (accidentally) flows through a different connection, such as a different Wi-Fi or cellular.
