//@Wuyang Zhang Communication Network ii, Project1, exercise2a

define($src_ip_addr 192.168.1.1,$dest_ip_addr 192.168.1.2)
define($src_mac_addr 3e:ee:d4:46:64:98, $dest_mac_addr 4e:8e:9f:e2:90:c8)
define($dev1 veth1, $dev2 veth2)


RatedSource(DATA "hello", RATE 1, LIMIT -1, STOP true)
		 ->Print("Source DATA")
		 ->IPEncap(4, $src_ip_addr, $dest_ip_addr)
		 ->Print("Finish IP Encap")
		 ->EtherEncap(0x0800, $src_mac_addr, $dest_mac_addr)
		 ->Print("Finish Ether Encap, Sending packet to dev1")	       
		 ->ToDevice($dev1)

FromDevice($dev2)

->Print("Receiving Packets from Dev2")->Discard
 		 



