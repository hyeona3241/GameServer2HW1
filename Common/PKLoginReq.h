#pragma once
#include "IPacket.h"
#include "PacketDef.h"
#include <vector>

#pragma pack(push,1)
struct PKLoginReqBody {
    char nickname[32];
};
#pragma pack(pop)

class PKLoginReq : public IPacket
{
public:
    PacketHeader   header;
    PKLoginReqBody body{};

    PKLoginReq();

    explicit PKLoginReq(const char* name);
    explicit PKLoginReq(const PacketHeader& h);

    std::vector<char> Serialize() const override;
    void Deserialize(const char* buffer, size_t sz) override;
    bool IsValid(const char* buffer, size_t sz) const override;
};

