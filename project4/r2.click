require(library /home/comnetsii/elements/routerport.click);

define($dev1 veth1, $addrDev1 4e:d9:46:f4:77:3f)
define($dev2 veth2, $addrDev2 fa:5a:05:e3:b9:f7)
define($dev3 veth3, $addrDev3 96:3b:82:98:0e:20)
define($dev4 veth4, $addrDev4 3a:e3:f7:73:67:5c)
define($dev5 veth5, $addrDev5 7a:31:49:b7:fa:0c)
define($dev6 veth6, $addrDev6 92:ca:27:70:9c:16)
define($dev7 veth7, $addrDev7 52:0c:c8:2b:87:39)
define($dev8 veth8, $addrDev8 26:77:2e:6d:27:5e)
define($dev9 veth9, $addrDev9 fe:b2:4d:ae:c1:06)
define($dev10 veth10, $addrDev10 b6:ab:84:ff:7b:41)
define($dev11 veth11, $addrDev11 d6:b2:b0:e7:1d:f0)
define($dev12 veth12, $addrDev12 7a:12:68:c0:38:45)


// ************************* define router output link ! **********************************************************************
rp0::RouterPort(DEV $dev4, IN_MAC $addrDev4, OUT_MAC $addrDev3)
rp1::RouterPort(DEV $dev5, IN_MAC $addrDev5, OUT_MAC $addrDev7)
rp2::RouterPort(DEV $dev6, IN_MAC $addrDev6, OUT_MAC $addrDev8)


cl::PacketClassifier()
ack::AckModule()

// ************************* @initiate address ! **********************************************************************
rt::RoutingTable(MY_ADDRESS 2)
hello::HelloModule(MY_ADDRESS 2, DELAY 1, PERIOD 5, TIME_OUT 2, ACK_TABLE ack, ROUTING_TABLE rt)
update::UpdateModule(MY_ADDRESS 2,DELAY 5, PERIOD 5, TIME_OUT 2,  ACK_TABLE ack, ROUTING_TABLE rt)

data::DataModule(ROUTING_TABLE rt)
bd::BroadcastModule()
//------------------------------------------------------------------------------------------------------------------------------

// ************************* @all input ports forward packets to Packet Classifier !*************************
rp0->[0]cl
rp1->[1]cl
rp2->[2]cl
// packet classifier outport 0: hello, outport1: update, outport2: ack, output3: data
// ack inport 0: receive ack packet, update ack table, inport 1: send out ack
cl[0]->hello
cl[1]->update
cl[2]->[0]ack 
cl[3]->data

// hello outport 0: generate hello packet, send to broadcast, outport 1: receive a hello packet, send ack 
hello[0]->bd
hello[1]->[1]ack 

// update outport 0: generate update packet, send to broadcast, outport 1: receive a update packet, send ack 
update[0]->bd
update[1]->[1]ack
// *************************@ack connect to all valid out port !*************************
ack[0]->rp0
ack[1]->rp1
ack[2]->rp2
// *************************@broadcast packet to all valid out ports !*************************
bd[0]->rp0
bd[1]->rp1
bd[2]->rp2
bd[3]->Discard
bd[4]->Discard

// *************************@forward packet to all valid out ports !*************************
data[0]->rp0
data[1]->rp1
data[2]->rp2