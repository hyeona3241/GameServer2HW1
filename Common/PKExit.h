#pragma once

#include "IPacket.h"
#include "PacketDef.h"
#include <vector>

class PKExit : public IPacket
{
public:
    PacketHeader header;

    PKExit();
    explicit PKExit(const PacketHeader& h);

    std::vector<char> Serialize() const override;
    void Deserialize(const char* buffer, size_t bufferSize) override;
    bool IsValid(const char* buffer, size_t bufferSize) const override;
};

