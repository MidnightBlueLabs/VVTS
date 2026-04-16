# VPN Vulnerability Testing Suite (VVTS)

VVTS is a tool that allows one to evaluate susceptibility of a VPN client to a wide variety of attacks. It does so by configuring and bringing up the attack infrastructure, in the form of a wireless access point, including the necessary services, such as DHCP, DNS, Router Advertisements and IPv6 to IPv4 address translation, and providing additional domain-specific functionality to the user such as ARP spoofing and traffic redirection. Subsequently, a victim device connects to said wireless access point, establishes a VPN connection, and retrieves a URL reported by VVTS in a QR code. Finally, VVTS monitors traffic in order to determine and report whether the URL was retrieved directly or through the VPN tunnel, indicating a successful or unsuccessful attack, respectively.

## Funding

This project is funded through [VPN Fund](https://nlnet.nl/thema/VPNFund.html), a fund established by [NLnet](https://nlnet.nl). Learn more at the [NLnet project page](https://nlnet.nl/project/VPN-vulnerabilitytesting).

[<img src="https://nlnet.nl/logo/banner.png" alt="NLnet foundation logo" width="20%" />](https://nlnet.nl)

## Documentation

Please refer to the [VVTS Documentation](https://github.com/MidnightBlueLabs/VVTS/blob/main/doc/index.md) to get started.