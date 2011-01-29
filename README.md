# ipcalc

If you sometimes find yourself scratching your head when dealing with IPv4
networks, then ipcalc might just be the tool for you. Ipcalc is a simple
rewrite of its homonym written in perl ipcalc. The main differences are that
this ipcalc is written in C, and supports ``BSD style'' netmasks written in
hexadecimal notation.

## features

Some interesting features of ipcalc (see manpage for more detail):

* netmask conversions between formats
* network splitting
* network finding
* no compile-time warnings, even with aggressive options

## manpage

[ipcalc(1)](http://spootnik.org/ipcalc/ipcalc.1.html "ipcalc manpage")

## examples
Getting information on a network:

    (pyr@phoenix) pyr$ipcalc 10.0.0.1/24 
    address   : 10.0.0.1        
    netmask   : 255.255.255.0   (0xffffff00)
    network   : 10.0.0.0        /24
    broadcast : 10.0.0.255      
    host min  : 10.0.0.1        
    host max  : 10.0.0.254      
    hosts/net : 254

Splitting a network into smaller networks

    (pyr@phoenix) pyr$ipcalc -vs 12,4 10.0.0.0/24
    you want a /28 to store 12 IPs
    address   : 10.0.0.0        
    netmask   : 255.255.255.240 (0xfffffff0)
    network   : 10.0.0.0        /28
    broadcast : 10.0.0.15       
    host min  : 10.0.0.1        
    host max  : 10.0.0.14       
    hosts/net : 14

    you want a /29 to store 4 IPs
    address   : 10.0.0.16       
    netmask   : 255.255.255.248 (0xfffffff8)
    network   : 10.0.0.16       /29
    broadcast : 10.0.0.23       
    host min  : 10.0.0.17       
    host max  : 10.0.0.22       
    hosts/net : 6

    remaining:
    10.0.0.24/29
    10.0.0.32/27
    10.0.0.64/26
    10.0.0.128/25
