require(library /home/comnetsii/elements/routerport.click);

define($dev1 veth1, $addrDev1 92:1b:0a:62:29:68)
define($dev2 veth2, $addrDev2 02:f8:d4:fb:fa:33)
define($dev3 veth3, $addrDev3 96:af:22:67:eb:29)
define($dev4 veth4, $addrDev4 fe:6b:59:31:53:c2)
define($dev5 veth5, $addrDev5 0e:59:ea:1f:38:2a)
define($dev6 veth6, $addrDev6 92:ca:27:70:9c:16)
define($dev7 veth7, $addrDev7 ce:b6:17:8c:98:71)
define($dev8 veth8, $addrDev8 a2:ce:56:e0:57:2c)
define($dev9 veth9, $addrDev9 d6:19:11:2e:1b:8d)
define($dev10 veth10, $addrDev10 0a:37:4c:6b:45:66)


rp0::RouterPort(DEV $dev1, IN_MAC $addrDev1, OUT_MAC $addrDev2)


cl::PacketClassifier()
hello::HelloModule(MY_ADDRESS 1, DELAY 0, PERIOD 5, TIME_OUT 2, ACK_TABLE ack, ROUTING_TABLE rt)
rt::RoutingTable()
update::UpdateModule(MY_ADDRESS 1,DELAY 0, PERIOD 5, TIME_OUT 2,  ACK_TABLE ack, ROUTING_TABLE rt)
bd::BroadcastModule()
ack::AckModule()


rp0->cl
cl[0]->hello
cl[1]->update
cl[2]->[0]ack //receive ack
cl[3]->Discard

hello[0]->bd
hello[1]->[1]ack // send ack

update[0]->bd
update[1]->[1]ack

ack[0]->rp0
bd->rp0