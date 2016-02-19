
require(library	/home/comnetsii/elements/routerport.click);

Idle->RouterPort(DEV $dev, IN_MAC $in_mac, OUT_MAC $out_mac)->ep :: Exercise2Packet

//ep[0]->RouterPort(DEV $dev2, OUT_MAC $out_mac, IN_MAC $in_mac)->Discard
ep[0]->RouterPort(DEV $dev, IN_MAC $in_mac, OUT_MAC $out_mac)->Discard
ep[1]->Discard



