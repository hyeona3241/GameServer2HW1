#pragma once

#include <vector>
#include <cstdint>
#include "PacketDef.h"

class IPacket
{
public:
    // 직렬화 : Packet -> 바이너리 버퍼
    virtual std::vector<char> Serialize() const = 0;
    // 역직렬화 : 바이너리 버퍼 -> Packet
    virtual void Deserialize(const char* buffer, size_t bufferSize) = 0;
    // 버퍼 유효성 체크
    virtual bool IsValid(const char* buffer, size_t bufferSize) const = 0;

    virtual ~IPacket() = default;
};

