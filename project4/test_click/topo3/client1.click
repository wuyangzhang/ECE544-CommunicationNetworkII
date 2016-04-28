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
define($dev13 veth13, $addrDev13 6e:5a:d3:a6:5d:46)
define($dev14 veth14, $addrDev14 be:a9:f2:1c:2b:7c)
define($dev15 veth15, $addrDev15 ee:fc:80:bf:42:c6)
define($dev16 veth16, $addrDev16 96:3e:e9:b6:a6:c2)
define($dev17 veth17, $addrDev17 7a:70:2f:1f:03:7b)
define($dev18 veth18, $addrDev18 36:07:37:5a:2a:ba)
define($dev19 veth19, $addrDev19 fe:34:e5:88:9b:da)
define($dev20 veth20, $addrDev20 2e:3e:88:bc:ff:a4)


//********************** Test Topology ***********************************
//
//
//                                     [veth4]Router2[veth5] 
//                                       /		        \
//                                      /				 \	
//                                [veth3]			   [veth6]
// Client0[veth1] -- [veth2]Router1							  Router3[veth7]----[veth8]Client1
//                                [veth12]			   [veth9]	
//                                     \				 /
//                                      \			    /									
//                                    [veth11]Router4[veth10] 							 [veth18]Client2
//													 [veth13]							    /
//														\								   /
//														 \								[veth17]
//													  [veth14]Router5[veth15]--[veth16]Router6
//                                                     					 				[veth19]
//																							\	
//																							 \
//																						   [veth20]Client3
//
// ************************* define access router output link ! //**********************************************************************
rp1::RouterPort(DEV $dev8, IN_MAC $addrDev8, OUT_MAC $addrDev7)

//client1::ClientModule(MY_ADDRESS 101, K 1, OTHER_ADDR1 100, OTHER_ADDR2 102, OTHER_ADDR3 103, DELAY 20, PERIOD 5, TIME_OUT 2)
//client1::ClientModule(MY_ADDRESS 101, K 2, OTHER_ADDR1 100, OTHER_ADDR2 102, OTHER_ADDR3 103, DELAY 20, PERIOD 5, TIME_OUT 2)
client1::ClientModule(MY_ADDRESS 101, K 3, OTHER_ADDR1 100, OTHER_ADDR2 102, OTHER_ADDR3 103, DELAY 20, PERIOD 5, TIME_OUT 2)

rp1->client1->rp1


