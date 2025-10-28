#include "PKExit.h"
#include <cstring>

PKExit::PKExit() 
    : header(PacketType::Exit, static_cast<uint16_t>(sizeof(PacketHeader) + sizeof(PKExitBody))) {}

PKExit::PKExit(const char* nick)
    : PKExit() {

    std::strncpy(body.nickname, nick ? nick : "", sizeof(body.nickname) - 1);
    body.nickname[sizeof(body.nickname) - 1] = '\0';
}

PKExit::PKExit(const PacketHeader& h) : header(h) {}

std::vector<char> PKExit::Serialize() const 
{
    std::vector<char> out(header.size);
    std::memcpy(out.data(), &header, sizeof(header));
    std::memcpy(out.data() + sizeof(header), &body, sizeof(body));
    return out;
}

void PKExit::Deserialize(const char* buffer, size_t bufferSize) 
{
    if (!IsValid(buffer, bufferSize)) return;
    std::memcpy(&header, buffer, sizeof(PacketHeader));
    std::memcpy(&body, buffer + sizeof(PacketHeader), sizeof(PKExitBody));
}

bool PKExit::IsValid(const char* buffer, size_t bufferSize) const 
{
    if (!buffer) return false;
    if (bufferSize < sizeof(PacketHeader) + sizeof(PKExitBody)) return false;

    const auto* h = reinterpret_cast<const PacketHeader*>(buffer);
    if (h->id != static_cast<uint16_t>(PacketType::Exit)) return false;
    if (h->size != sizeof(PacketHeader) + sizeof(PKExitBody)) return false;

    return true;
}