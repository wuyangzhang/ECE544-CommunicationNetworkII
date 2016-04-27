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
// ************************* define access router output link ! 
//**********************************************************************
rp0::RouterPort(DEV $dev1, IN_MAC $addrDev1, OUT_MAC $addrDev2)

//client0::ClientModule(MY_ADDRESS 100, K 1, OTHER_ADDR1 101, OTHER_ADDR2 102, OTHER_ADDR3 103,DELAY 20, PERIOD 5, TIME_OUT 2)
//client0::ClientModule(MY_ADDRESS 100, K 2, OTHER_ADDR1 101, OTHER_ADDR2 102, OTHER_ADDR3 103,DELAY 20, PERIOD 5, TIME_OUT 2)
client0::ClientModule(MY_ADDRESS 101, K 3, OTHER_ADDR1 100, OTHER_ADDR2 102, OTHER_ADDR3 103,DELAY 20, PERIOD 5, TIME_OUT 2)

rp0->client0->rp0


