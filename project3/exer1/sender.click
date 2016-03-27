
require(library	/home/comnetsii/elements/routerport.click);

define($dev veth1, $in_mac 7e:e4:78:41:51:a5, $out_mac ee:ab:2d:44:8e:1f)
rp::RouterPort(DEV $dev, IN_MAC $in_mac, OUT_MAC $out_mac)

//Idle->rp->PacketSender()->rp
rp->PacketSender()->rp



