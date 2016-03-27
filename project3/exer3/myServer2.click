// click file for client

require(library	  /home/comnetsii/elements/routerport.click);
define($dev1 veth1, $addrDev1 e2:e5:a8:a4:12:81)
define($dev2 veth2, $addrDev2 4a:26:97:74:af:2c)
define($dev3 veth3, $addrDev3 3e:65:38:79:bf:9b)
define($dev4 veth4, $addrDev4 ea:21:81:06:62:10)
define($dev5 veth5, $addrDev5 26:57:6a:f5:2d:13)
define($dev6 veth6, $addrDev6 92:9c:22:8a:f8:69)

rp::RouterPort(DEV $dev6, IN_MAC $addrDev6, OUT_MAC $addrDev5)

rp->PacketServer(SrcAddr 103)->rp

