#pragma once
#include "IPacket.h"
#include "PacketDef.h"
#include <vector>

class PKUserCount : public IPacket
{
public:
    PacketHeader header;

    PKUserCount();
    explicit PKUserCount(const PacketHeader& h);

    std::vector<char> Serialize() const override;
    void Deserialize(const char* buffer, size_t bufferSize) override;
    bool IsValid(const char* buffer, size_t bufferSize) const override;
};

