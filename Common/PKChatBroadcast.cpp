#include "PKChatBroadcast.h"

PKChatBroadcast::PKChatBroadcast()
	: header(PacketType::ChatBroadcast, (uint16_t)(sizeof(PacketHeader) + sizeof(PKChatBroadcastBody))) {}

PKChatBroadcast::PKChatBroadcast(const char* nick, const char* msg, uint64_t ts)
    : PKChatBroadcast() {
    std::strncpy(body.sender, nick ? nick : "", sizeof(body.sender) - 1);
    body.sender[sizeof(body.sender) - 1] = '\0';

    std::strncpy(body.message, msg ? msg : "", sizeof(body.message) - 1);
    body.message[sizeof(body.message) - 1] = '\0';
    body.timestampMs = ts;
}

std::vector<char> PKChatBroadcast::Serialize() const
{
    std::vector<char> out(header.size);
    std::memcpy(out.data(), &header, sizeof(header));
    std::memcpy(out.data() + sizeof(header), &body, sizeof(body));
    return out;
}

void PKChatBroadcast::Deserialize(const char* buf, size_t sz)
{
    if (!IsValid(buf, sz)) return;
    std::memcpy(&header, buf, sizeof(PacketHeader));
    std::memcpy(&body, buf + sizeof(PacketHeader), sizeof(PKChatBroadcastBody));
}

bool PKChatBroadcast::IsValid(const char* buf, size_t sz) const
{
    if (!buf) return false;
    if (sz < sizeof(PacketHeader) + sizeof(PKChatBroadcastBody)) return false;

    auto* h = reinterpret_cast<const PacketHeader*>(buf);

    return h->id == (uint16_t)PacketType::ChatBroadcast && h->size == sizeof(PacketHeader) + sizeof(PKChatBroadcastBody);
}
