/* Create by Yu Sun */

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

This topo is to test when there are two ports have the same cost.

How would router compute its table? Does it allow duplicate vlues?