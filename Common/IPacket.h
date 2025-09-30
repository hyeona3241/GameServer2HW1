#pragma once

#include <vector>
#include <cstdint>
#include "PacketDef.h"

class IPacket
{
public:
    // ����ȭ : Packet -> ���̳ʸ� ����
    virtual std::vector<char> Serialize() const = 0;
    // ������ȭ : ���̳ʸ� ���� -> Packet
    virtual void Deserialize(const char* buffer, size_t bufferSize) = 0;
    // ���� ��ȿ�� üũ
    virtual bool IsValid(const char* buffer, size_t bufferSize) const = 0;

    virtual ~IPacket() = default;
};

