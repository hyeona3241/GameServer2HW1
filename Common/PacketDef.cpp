#include "PacketDef.h"
#include <cstring>

ChatMessagePacket ToPacket(const ChatMessageData& data) noexcept {
    ChatMessagePacket pkt{};
    pkt.header = PacketHeader(PacketType::ChatMessage, sizeof(ChatMessagePacket));

    strncpy_s(pkt.sender, data.sender.c_str(), sizeof(pkt.sender) - 1);
    strncpy_s(pkt.message, data.message.c_str(), sizeof(pkt.message) - 1);

    pkt.timestamp = data.timestamp;
    return pkt;
}

ChatMessageData ToData(const ChatMessagePacket& pkt) {
    ChatMessageData data;

    data.sender = pkt.sender;
    data.message = pkt.message;
    data.timestamp = pkt.timestamp;

    return data;
}