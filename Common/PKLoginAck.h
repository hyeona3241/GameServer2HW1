#pragma once
#include "IPacket.h"
#include "PacketDef.h"
#include <vector>

#pragma pack(push,1)
struct PKLoginAckBody {
    uint64_t sessionId;
    uint8_t accepted; 
    char reason[64];    // ���� ����(�����ϸ� �� ���ڿ�)
    int32_t currentUsers;   //  ������ ��
};
#pragma pack(pop)

class PKLoginAck : public IPacket
{
public:
    PacketHeader   header;
    PKLoginAckBody body{};

    PKLoginAck();

    PKLoginAck(uint64_t sid, bool ok, const char* why, int32_t curUsers);
    explicit PKLoginAck(const PacketHeader& h);

    std::vector<char> Serialize() const override;
    void Deserialize(const char* buffer, size_t sz) override;
    bool IsValid(const char* buffer, size_t sz) const override;
};

