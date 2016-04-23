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

// ************************* define access router output link ! **********************************************************************
rp0::RouterPort(DEV $dev1, IN_MAC $addrDev1, OUT_MAC $addrDev2)

client::ClientModule(MY_ADDRESS 101, K 2, OTHER_ADDR1 102, OTHER_ADDR2 103, DELAY 20, PERIOD 5, TIME_OUT 2)
//client::ClientModule(MY_ADDRESS 101,  K 3, OTHER_ADDR1 101, OTHER_ADDR2 102, OTHER_ADDR3 103, DELAY 20, PERIOD 5, TIME_OUT 2)

rp0->client->rp0


