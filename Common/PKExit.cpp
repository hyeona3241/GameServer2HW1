#include "PKExit.h"
#include <cstring>

PKExit::PKExit() {
    header = PacketHeader(PacketType::Exit,
        static_cast<uint16_t>(sizeof(PacketHeader)));
}

PKExit::PKExit(const PacketHeader& h) : header(h) { }

std::vector<char> PKExit::Serialize() const {
    std::vector<char> out(sizeof(PacketHeader));
    std::memcpy(out.data(), &header, sizeof(PacketHeader));
    return out;
}

void PKExit::Deserialize(const char* buffer, size_t bufferSize) {
    if (!IsValid(buffer, bufferSize)) return;
    std::memcpy(&header, buffer, sizeof(PacketHeader));
}

bool PKExit::IsValid(const char* buffer, size_t bufferSize) const {
    if (!buffer) return false;
    if (bufferSize < sizeof(PacketHeader)) return false;

    const auto* h = reinterpret_cast<const PacketHeader*>(buffer);
    if (h->id != static_cast<uint16_t>(PacketType::Exit)) return false;
    if (h->size != sizeof(PacketHeader)) return false;

    return true;
}