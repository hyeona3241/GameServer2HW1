#pragma once

#include "IPacket.h"
#include "PacketDef.h"
#include <vector>

class PKChatMessage : IPacket
{
public:
    ChatMessagePacket pkt;

    PKChatMessage();
    explicit PKChatMessage(const ChatMessageData& data);

    void SetData(const ChatMessageData& data);
    ChatMessageData GetData() const;

    std::vector<char> Serialize() const override;
    void Deserialize(const char* buffer, size_t bufferSize) override;
    bool IsValid(const char* buffer, size_t bufferSize) const override;
};

