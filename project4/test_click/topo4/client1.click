require(library /home/comnetsii/elements/routerport.click);

define($dev1 veth1, $addrDev1 1e:9e:64:0a:54:bd)
define($dev2 veth2, $addrDev2 92:11:66:96:0a:4a)
define($dev3 veth3, $addrDev3 3a:7d:75:61:cc:fa)
define($dev4 veth4, $addrDev4 de:65:c9:c7:aa:29)
define($dev5 veth5, $addrDev5 66:17:6c:45:c9:06)
define($dev6 veth6, $addrDev6 0e:13:20:1d:d4:f3)
define($dev7 veth7, $addrDev7 92:21:63:40:56:e7)
define($dev8 veth8, $addrDev8 5e:af:8d:9a:5d:69)
define($dev9 veth9, $addrDev9 ae:41:f5:cc:c9:04)
define($dev10 veth10, $addrDev10 5a:f9:89:60:3b:34)
define($dev11 veth11, $addrDev11 ee:2d:4c:45:40:3a)
define($dev12 veth12, $addrDev12 8e:84:a1:0a:97:0e)
define($dev13 veth13, $addrDev13 f2:32:2a:2d:b7:5f)
define($dev14 veth14, $addrDev14 42:56:e4:5f:01:57)
define($dev15 veth15, $addrDev15 8a:74:3e:3b:89:d3)
define($dev16 veth16, $addrDev16 4e:e2:4f:e7:e0:ab)
define($dev17 veth17, $addrDev17 aa:81:ae:ae:80:12)
define($dev18 veth18, $addrDev18 3a:51:b8:e3:a2:cf)
define($dev19 veth19, $addrDev19 f2:61:c7:e9:37:39)
define($dev20 veth20, $addrDev20 9a:68:0b:e3:50:de)
define($dev21 veth21, $addrDev21 ca:13:f6:c6:21:d8)
define($dev22 veth22, $addrDev22 ae:30:4f:0d:22:a6)
define($dev23 veth23, $addrDev23 3a:cc:21:5b:5f:fe)
define($dev24 veth24, $addrDev24 56:02:ed:8c:9a:9c)



//********************** Test Topology ***********************************
//
//
//                                        ----[veth4]Router2[veth9]--------[veth10]Router5[veth15]---[veth16]Client1 
//                                       /		                                   [veth21]  
//                                      /				 				              |
//                                 [veth3]			                               [veth22]
// Client0[veth1] -- [veth2]Router1[veth5]----[veth6]Router3[veth11]-------[veth12]Router6[veth17]---[veth18]Client2
//                                 [veth7]										   [veth23]					
//									  \				          						  |
//                                     \										   [veth24]
//										------[veth8]Router4[veth13]-------[veth14]Router5[veth19]---[veth20]Client3
//
// ************************* define access router output link ! //**********************************************************************
rp1::RouterPort(DEV $dev16, IN_MAC $addrDev16, OUT_MAC $addrDev15)

//client1::ClientModule(MY_ADDRESS 101, K 1, OTHER_ADDR1 100, OTHER_ADDR2 102, OTHER_ADDR3 103, DELAY 20, PERIOD 5, TIME_OUT 2)
//client1::ClientModule(MY_ADDRESS 101, K 2, OTHER_ADDR1 100, OTHER_ADDR2 102, OTHER_ADDR3 103, DELAY 20, PERIOD 5, TIME_OUT 2)
client1::ClientModule(MY_ADDRESS 101, K 3, OTHER_ADDR1 100, OTHER_ADDR2 102, OTHER_ADDR3 103, DELAY 20, PERIOD 5, TIME_OUT 2)

rp1->client1->rp1


