struct PacketHeader{
	uint8_t type; // type 0 -> data, type 1 -> ack
	uint32_t destAddr;
	uint32_t srcAddr;
	uint32_t sequenceNumber;
}