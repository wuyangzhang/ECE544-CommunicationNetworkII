require(library /home/comnetsii/elements/routerport.click);

define($dev1 veth1, $addrDev1 62:61:d7:f7:80:d5)
define($dev2 veth2, $addrDev2 5a:7d:c9:73:01:3e)
define($dev3 veth3, $addrDev3 12:d0:07:6e:4e:6b)
define($dev4 veth4, $addrDev4 de:07:94:99:5f:5b)
define($dev5 veth5, $addrDev5 72:9e:c8:95:69:3f)
define($dev6 veth6, $addrDev6 76:28:e7:41:6f:12)
define($dev7 veth7, $addrDev7 e2:33:b5:86:c7:03)
define($dev8 veth8, $addrDev8 d2:d2:b3:83:70:2f)
define($dev9 veth9, $addrDev9 6a:70:14:03:21:0c)
define($dev10 veth10, $addrDev10 8a:f3:a4:3c:8b:c7)
define($dev11 veth11, $addrDev11 be:44:d1:b4:81:7e)
define($dev12 veth12, $addrDev12 7e:1f:44:c9:15:ee)


//********************** Test Topology ***********************************
//
//
//                                                    --- [veth4] Router2 [veth5] 
//                                                   /							\
//                                                  /							 \	
//                                          [veth3]								[veth6]
//       Client0 [veth1] ----- [veth2] Router1										Router3[veth7]----[veth8]Client1
//                                           [veth12]							[veth9]	
//                                                  \							  /
//                                                    \							 /	
//                                                     --- [veth11] Router4 [veth10] 
//                                                     					 
//

// ************************* define router output link ! **********************************************************************
rp0::RouterPort(DEV $dev2, IN_MAC $addrDev2, OUT_MAC $addrDev1)
rp1::RouterPort(DEV $dev3, IN_MAC $addrDev3, OUT_MAC $addrDev4)
rp2::RouterPort(DEV $dev12, IN_MAC $addrDev12, OUT_MAC $addrDev11)


cl::PacketClassifier()
ack::AckModule()
// ************************* @initiate address ! **********************************************************************
rt::RoutingTable(MY_ADDRESS 1)
hello::HelloModule(MY_ADDRESS 1, DELAY 1, PERIOD 5, TIME_OUT 2, ACK_TABLE ack, ROUTING_TABLE rt)
update::UpdateModule(MY_ADDRESS 1,DELAY 5, PERIOD 5, TIME_OUT 2,  ACK_TABLE ack, ROUTING_TABLE rt)
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

// *************************@broadcast packet to all valid out ports, except access router************************
bd[0]->Discard
bd[1]->rp1
bd[2]->rp2
bd[3]->Discard
bd[4]->Discard

// *************************@forward packet to all valid out ports !*************************
data[0]->rp0
data[1]->rp1
data[2]->rp2
