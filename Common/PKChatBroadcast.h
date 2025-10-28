#pragma once
#include "IPacket.h"

#pragma pack(push,1)
struct PKChatBroadcastBody {
	char sender[32];
	char message[256];
	uint64_t timestampMs;
};
#pragma pack(pop)

class PKChatBroadcast : public IPacket
{
public:
    PacketHeader header;
    PKChatBroadcastBody body{};

    PKChatBroadcast();

    PKChatBroadcast(const char* nick, const char* msg, uint64_t ts = 0);

    std::vector<char> Serialize() const override;
    void Deserialize(const char* buf, size_t sz) override;
    bool IsValid(const char* buf, size_t sz) const override;
};

