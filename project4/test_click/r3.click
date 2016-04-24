require(library /home/comnetsii/elements/routerport.click);

define($dev1 veth1, $addrDev1 e6:12:0b:b5:38:c5)
define($dev2 veth2, $addrDev2 26:16:54:22:10:87)
define($dev3 veth3, $addrDev3 02:ae:8b:cb:19:54)
define($dev4 veth4, $addrDev4 46:0a:ad:c9:68:c0)
define($dev5 veth5, $addrDev5 5e:fa:80:6a:31:c5)
define($dev6 veth6, $addrDev6 fa:12:a8:22:12:93)
define($dev7 veth7, $addrDev7 aa:15:1c:4e:64:44)
define($dev8 veth8, $addrDev8 de:69:81:38:c0:6e)
define($dev9 veth9, $addrDev9 9a:51:04:43:dd:81)
define($dev10 veth10, $addrDev10 12:05:2a:19:1e:6b)
define($dev11 veth11, $addrDev11 9e:b2:17:c6:7f:7b)
define($dev12 veth12, $addrDev12 4e:c0:fe:06:76:f5)


//********************** Test Topology ***********************************
//
//
//                                                  --- [veth4] Router2 [veth5] ----- [veth6] Client1
//                                                   /
//                                                  /
//                                          [veth3]
//       Client0 [veth1] ----- [veth2] Router1
//                                           [veth7]
//                                                  \
//                                                    \
//                                                     --- [veth8] Router3 [veth9] ----- [veth10] Client2
//                                                     					 [veth11]-----[veth12] Client3
//
// ************************* define router output link ! **********************************************************************
rp0::RouterPort(DEV $dev8, IN_MAC $addrDev8, OUT_MAC $addrDev7)
rp1::RouterPort(DEV $dev9, IN_MAC $addrDev9, OUT_MAC $addrDev10)
rp2::RouterPort(DEV $dev11, IN_MAC $addrDev11, OUT_MAC $addrDev12)


cl::PacketClassifier()
ack::AckModule()

// ************************* @initiate address ! **********************************************************************
rt::RoutingTable(MY_ADDRESS 3)
hello::HelloModule(MY_ADDRESS 3, DELAY 1, PERIOD 5, TIME_OUT 2, ACK_TABLE ack, ROUTING_TABLE rt)
update::UpdateModule(MY_ADDRESS 3,DELAY 5, PERIOD 5, TIME_OUT 2,  ACK_TABLE ack, ROUTING_TABLE rt)

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
bd[1]->Discard
bd[2]->Discard
bd[3]->Discard
bd[4]->Discard
// *************************@forward packet to all valid out ports !*************************
data[0]->rp0
data[1]->rp1
data[2]->rp2