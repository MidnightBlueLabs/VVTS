# `miscnet`

The `miscnet` class provides miscellaneous network functionality. It is instantiated as follows:
```
miscnet = miscnet();
```

## Methods

### `subnet_ip(ip_addr, netmask)`
Returns a string containing the first IPv4 address in the subnet defined by IPv4 address `ip_addr` and netmask `netmask`, i.e. a bitwise AND of the two values encoded as an IPv4 address. Both arguments are strings containing an IPv4 address.

### `subnet_ipv6(ipv6_addr, prefixlen)`
Returns a string containing the first IPv6 address in the subnet defined by IPv6 address `ipv6_addr` and prefix length `prefixlen`, i.e. all bits beyond the first `prefixlen` bits set to zero, encoded as an IPv6 address. `ipv6_addr` is a string containing an IPv6 address, `prefixlen` is a string containing the number `0` up to `128`.

### `lookup_ipv4(domain_name)`
Resolves the IPv4 address of host `domain_name` through DNS. Returns a string containing the resulting IPv4 address. `domain_name` is a string containing the domain name to be resolved.

### `lookup_ipv6(domain_name)`
Resolves the IPv6 address of host `domain_name` through DNS. Returns a string containing the resulting IPv6 address. `domain_name` is a string containing the domain name to be resolved.

### `masquerade(value)`
`value` is string containing a Boolean value determining whether masquerading is to be enabled. Setting this to `true` causes traffic with a destination outside of the local subnet to be forwarded to our default router in a  masqueraded fashion, i.e. pretending to be coming from us.

### `set_redirect(from, to)`
Causes traffic intended for `from` to be redirected to `to`, rather than simply being forwarded. Both `from` and `to` are strings of the format
```
[proto]://[hostname]:[port]
```
Where `proto` is either `tcp` or `udp`. Hostname can be either an IPv4 or IPv6 address, or a domain name that is subsequently resolved.

Alternatively, one may provide input strings of the format
```
[proto]://[hostname]
```
Where `proto` is either `gre`, `ipsec` or `l2tp`. As these protocols do not have a concept of a port, no port number is to be provided.

Finally, input strings of the format
```
[hostname]
```
causes a blanket redirect for any protocol and any port to be instantiated. If a blanket redirect is deemed too broad, the `no_redirect()` method can be used to create exceptions for certain protocols and/or ports.

### `no_redirect(from)`
Creates an exception for traffic redirection rules set through `set_redirect()`. Traffic intended for `from` is forwarded, rather than redirected. The `from` argument is a string of the exact same format as those provided to `set_redirect()` (see `set_redirect()`).
