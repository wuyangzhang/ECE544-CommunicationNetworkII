/**
  * Created by Wuyang on 4/19/16.
  * Copyright © 2016 Wuyang. All rights reserved.
  * Final Project of ECE 544 Communication Network II 2016 Spring
  * --------------------------------------------
  * Define packet semantics:
  * 	Hello = 1
  * 	Update = 2
  * 	Acknowledge = 3
  * 	Data = 4
 */


  typedef enum {
  	HELLO = 1,
  	UPDATE,
  	ACK,
  	DATA
  } packet_types;

  struct PacketType{
  	uint8_t type;
  };

  struct HelloPacket{
  	uint8_t type;
  	uint16_t sourceAddr;
  	uint8_t sequenceNumber;
  };

  struct UpdatePacket{
  	uint8_t type;
  	uint16_t sourceAddr;
  	uint8_t sequenceNumber;
  	uint16_t length;
  };

  struct AckPacket{
	uint8_t type;
  	uint16_t sourceAddr;
  	uint8_t sequenceNumber;
  	uint16_t destinationAddr;
  };

  struct DataPacket{
  	uint8_t type;
  	uint16_t sourceAddr;
  	uint8_t k_value;
  	uint8_t sequenceNumber;
  	uint16_t destinationAddr1;
  	uint16_t destinationAddr2;
  	uint16_t destinationAddr3;
  	uint16_t length;
  };