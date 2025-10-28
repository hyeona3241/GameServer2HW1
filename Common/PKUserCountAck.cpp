#include "PKUserCountAck.h"
#include <cstring>

PKUserCountAck::PKUserCountAck() 
	: header(PacketType::UserCountAck, static_cast<uint16_t>(sizeof(PacketHeader) + sizeof(PKUserCountAckBody))) {}


PKUserCountAck::PKUserCountAck(int32_t c) 
	: header(PacketType::UserCountAck, static_cast<uint16_t>(sizeof(PacketHeader) + sizeof(PKUserCountAckBody))) {
	body.count = c;
}

PKUserCountAck::PKUserCountAck(const PacketHeader& h) : header(h) {}

std::vector<char> PKUserCountAck::Serialize() const
{
	std::vector<char> out(header.size);
	std::memcpy(out.data(), &header, sizeof(header));
	std::memcpy(out.data() + sizeof(header), &body, sizeof(body));
	return out;
}

void PKUserCountAck::Deserialize(const char* buffer, size_t sz)
{
	if (!IsValid(buffer, sz)) return;
	std::memcpy(&header, buffer, sizeof(PacketHeader));
	std::memcpy(&body, buffer + sizeof(PacketHeader), sizeof(PKUserCountAckBody));
}

bool PKUserCountAck::IsValid(const char* buffer, size_t sz) const
{
	if (!buffer) return false;
	if (sz < sizeof(PacketHeader) + sizeof(PKUserCountAckBody)) return false;

	const auto* h = reinterpret_cast<const PacketHeader*>(buffer);
	if (h->id != static_cast<uint16_t>(PacketType::UserCountAck)) return false;
	if (h->size != sizeof(PacketHeader) + sizeof(PKUserCountAckBody)) return false;

	return true;
}
