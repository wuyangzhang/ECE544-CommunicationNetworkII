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

// ************************* define access router output link ! **********************************************************************
rp0::RouterPort(DEV $dev10, IN_MAC $addrDev10, OUT_MAC $addrDev9)

client::ClientModule(MY_ADDRESS 102, K 2, OTHER_ADDR1 101, OTHER_ADDR2 103, DELAY 20, PERIOD 5, TIME_OUT 2)
//client::ClientModule(MY_ADDRESS 103,  K 3, OTHER_ADDR1 101, OTHER_ADDR2 102, OTHER_ADDR3 103, DELAY 20, PERIOD 5, TIME_OUT 2)

rp0->client->rp0


