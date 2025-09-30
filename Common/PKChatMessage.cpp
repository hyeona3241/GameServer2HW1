#include "PKChatMessage.h"
#include <cstring>

PKChatMessage::PKChatMessage() {
    pkt.header = PacketHeader(PacketType::ChatMessage, static_cast<uint16_t>(sizeof(ChatMessagePacket)));
    pkt.sender[0] = '\0';
    pkt.message[0] = '\0';
    pkt.timestamp = 0;
}

PKChatMessage::PKChatMessage(const ChatMessageData& data) {
    pkt = ToPacket(data);
    pkt.header = PacketHeader(PacketType::ChatMessage,
        static_cast<uint16_t>(sizeof(ChatMessagePacket)));
}

void PKChatMessage::SetData(const ChatMessageData& data) {
    pkt = ToPacket(data);
    pkt.header = PacketHeader(PacketType::ChatMessage,
        static_cast<uint16_t>(sizeof(ChatMessagePacket)));
}

ChatMessageData PKChatMessage::GetData() const {
    return ToData(pkt);
}

std::vector<char> PKChatMessage::Serialize() const {
    std::vector<char> buffer(sizeof(ChatMessagePacket));
    std::memcpy(buffer.data(), &pkt, sizeof(ChatMessagePacket));
    return buffer;
}

void PKChatMessage::Deserialize(const char* buffer, size_t bufferSize) {
    if (!IsValid(buffer, bufferSize)) return;
    std::memcpy(&pkt, buffer, sizeof(ChatMessagePacket));

    pkt.sender[sizeof(pkt.sender) - 1] = '\0';
    pkt.message[sizeof(pkt.message) - 1] = '\0';
}

bool PKChatMessage::IsValid(const char* buffer, size_t bufferSize) const {
    if (!buffer) return false;
    if (bufferSize < sizeof(ChatMessagePacket)) return false;

    const auto* h = reinterpret_cast<const PacketHeader*>(buffer);
    if (h->id != static_cast<uint16_t>(PacketType::ChatMessage)) return false;
    if (h->size != sizeof(ChatMessagePacket)) return false;

    return true;
}