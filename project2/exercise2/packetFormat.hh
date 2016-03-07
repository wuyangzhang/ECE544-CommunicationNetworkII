#ifndef CLICK_PACKETFORMAT_HH
#define CLICK_PACKETFORMAT_HH

CLICK_DECLS

class PacketFormat{
public:
	struct selfDefinedPacketHead{
		uint8_t type;
		uint32_t length;
	};
	struct selfDefinedPacketPayload{
		const char* payload;
	};
};

CLICK_ENDDECLS
#endif
