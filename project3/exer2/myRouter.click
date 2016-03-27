// Router 
// topology C1 -> R1 -> C2
					 -> C3

require(library	/home/comnetsii/elements/routerport.click);

define($dev1 veth1, $addrDev1 e2:e5:a8:a4:12:81)
define($dev2 veth2, $addrDev2 4a:26:97:74:af:2c)
define($dev3 veth3, $addrDev3 3e:65:38:79:bf:9b)
define($dev4 veth4, $addrDev4 ea:21:81:06:62:10)
define($dev5 veth5, $addrDev5 26:57:6a:f5:2d:13)
define($dev6 veth6, $addrDev6 92:9c:22:8a:f8:69)

define($dev veth1, $in_mac e2:e5:a8:a4:12:81, $out_mac 4a:26:97:74:af:2c) 

rp1::RouterPort(DEV $dev2, IN_MAC $addrDev2, OUT_MAC $addrDev1)
rp2::RouterPort(DEV $dev3, IN_MAC $addrDev3, OUT_MAC $addrDev4)
rp3::RouterPort(DEV $dev5, IN_MAC $addrDev5, OUT_MAC $addrDev6)

//define router that has 3 outputs. 
myrouter::myRouter()

rp1->myrouter[0]->rp1
rp2->myrouter[1]->rp2
rp3->myrouter[2]->rp3




