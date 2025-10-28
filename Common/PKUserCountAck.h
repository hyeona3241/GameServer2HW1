#pragma once
#include "IPacket.h"
#include "PacketDef.h"
#include <vector>

#pragma pack(push,1)
struct PKUserCountAckBody {
    int32_t count;
};
#pragma pack(pop)

class PKUserCountAck : public IPacket
{
public:
    PacketHeader header;
    PKUserCountAckBody body{};

    PKUserCountAck();

    explicit PKUserCountAck(int32_t c);
    explicit PKUserCountAck(const PacketHeader& h);

    std::vector<char> Serialize() const override;
    void Deserialize(const char* buffer, size_t sz) override;
    bool IsValid(const char* buffer, size_t sz) const override;
};

