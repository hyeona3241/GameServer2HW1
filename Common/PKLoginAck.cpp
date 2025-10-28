#include "PKLoginAck.h"
#include <cstring>
#include <cstdint>

PKLoginAck::PKLoginAck()
	: header(PacketType::LoginAck, static_cast<uint16_t>(sizeof(PacketHeader) + sizeof(PKLoginAckBody))) {}

PKLoginAck::PKLoginAck(uint64_t sid, bool ok, const char* why, int32_t curUsers)
    : PKLoginAck() {
    body.sessionId = sid;
    body.accepted = ok ? 1u : 0u;
    std::strncpy(body.reason, (why ? why : ""), sizeof(body.reason));
    body.reason[sizeof(body.reason) - 1] = '\0';
    body.currentUsers = curUsers;
}

PKLoginAck::PKLoginAck(const PacketHeader& h) : header(h) {}

std::vector<char> PKLoginAck::Serialize() const
{
    std::vector<char> out(header.size);
    std::memcpy(out.data(), &header, sizeof(header));
    std::memcpy(out.data() + sizeof(header), &body, sizeof(body));
    return out;
}

void PKLoginAck::Deserialize(const char* buffer, size_t sz) 
{
    if (!IsValid(buffer, sz)) return;
    std::memcpy(&header, buffer, sizeof(PacketHeader));
    std::memcpy(&body, buffer + sizeof(PacketHeader), sizeof(PKLoginAckBody));
}

bool PKLoginAck::IsValid(const char* buffer, size_t sz) const 
{
    if (!buffer) return false;
    if (sz < sizeof(PacketHeader) + sizeof(PKLoginAckBody)) return false;

    const auto* h = reinterpret_cast<const PacketHeader*>(buffer);
    if (h->id != static_cast<uint16_t>(PacketType::LoginAck)) return false;
    if (h->size != sizeof(PacketHeader) + sizeof(PKLoginAckBody)) return false;

    return true;
}
