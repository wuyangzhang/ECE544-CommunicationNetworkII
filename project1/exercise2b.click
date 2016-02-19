//@Wuyang Zhang Communication Network ii, Project1, exercise2b

define($src_ip_addr 192.168.1.1,$dest_ip_addr 192.168.1.2)
define($src_mac_addr 3e:ee:d4:46:64:98, $dest_mac_addr 4e:8e:9f:e2:90:c8)
define($dev1 veth1, $dev2 veth2)


FromDevice($dev2)
->Print("ReceivingPakcet")->Strip(14) // strip ether header
->Print("StripEntherHeader")
->Strip(20)->Print("Receiving Packets From dev2", CONTENTS ASCII) //strip ip header
->Queue
->IPEncap(4, $dest_ip_addr, $src_ip_addr)
->EtherEncap(0x0800, $dest_mac_addr, $src_mac_addr)
->Print("Sending Packets to Dev2")
->ToDevice($dev2)