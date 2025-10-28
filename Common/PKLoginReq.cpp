#include "PKLoginReq.h"
#include <cstring>

PKLoginReq::PKLoginReq() 
	: header(PacketType::LoginReq, static_cast<uint16_t>(sizeof(PacketHeader) + sizeof(PKLoginReqBody))) {}


PKLoginReq::PKLoginReq(const char* name)
	: PKLoginReq() {
	std::strncpy(body.nickname, name ? name : "", sizeof(body.nickname));
	body.nickname[sizeof(body.nickname) - 1] = '\0';
}

PKLoginReq::PKLoginReq(const PacketHeader& h) : header(h) {}

std::vector<char> PKLoginReq::Serialize() const
{
	std::vector<char> out(header.size);
	std::memcpy(out.data(), &header, sizeof(header));
	std::memcpy(out.data() + sizeof(header), &body, sizeof(body));
	return out;
}

void PKLoginReq::Deserialize(const char* buffer, size_t sz)
{
	if (!IsValid(buffer, sz)) return;
	std::memcpy(&header, buffer, sizeof(PacketHeader));
	std::memcpy(&body, buffer + sizeof(PacketHeader), sizeof(PKLoginReqBody));
}

bool PKLoginReq::IsValid(const char* buffer, size_t sz) const
{
	if (!buffer) return false;
	if (sz < sizeof(PacketHeader) + sizeof(PKLoginReqBody)) return false;

	const auto* h = reinterpret_cast<const PacketHeader*>(buffer);
	if (h->id != static_cast<uint16_t>(PacketType::LoginReq)) return false;
	if (h->size != sizeof(PacketHeader) + sizeof(PKLoginReqBody)) return false;

	return true;
}
