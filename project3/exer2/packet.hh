struct PacketHeader{
	uint8_t type; // type 0 -> data, type 1 -> ack , type 2-> hello
	String destAddr;
	String srcAddr;
	uint32_t sequenceNumber;
};


