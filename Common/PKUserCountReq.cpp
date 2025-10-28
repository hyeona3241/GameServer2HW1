#include "PKUserCountReq.h"
#include <cstring>


PKUserCountReq::PKUserCountReq() 
    : header(PacketType::UserCountReq, static_cast<uint16_t>(sizeof(PacketHeader))) {}

PKUserCountReq::PKUserCountReq(const PacketHeader& h) : header(h) {}

std::vector<char> PKUserCountReq::Serialize() const {
    std::vector<char> out(sizeof(PacketHeader));
    std::memcpy(out.data(), &header, sizeof(PacketHeader));
    return out;
}

void PKUserCountReq::Deserialize(const char* buffer, size_t bufferSize) {
    if (!IsValid(buffer, bufferSize)) return;
    std::memcpy(&header, buffer, sizeof(PacketHeader));
}

bool PKUserCountReq::IsValid(const char* buffer, size_t bufferSize) const {
    if (!buffer) return false;
    if (bufferSize < sizeof(PacketHeader)) return false;

    const auto* h = reinterpret_cast<const PacketHeader*>(buffer);
    if (h->id != static_cast<uint16_t>(PacketType::UserCount)) return false;
    if (h->size != sizeof(PacketHeader)) return false;

    return true;
}