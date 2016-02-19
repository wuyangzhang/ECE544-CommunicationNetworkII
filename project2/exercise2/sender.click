
require(library	/home/comnetsii/elements/routerport.click);

Idle->GeneratePacket(MAXPACKETSIZE 100)->RouterPort(DEV $dev, IN_MAC $in_mac, OUT_MAC $out_mac)->Discard


