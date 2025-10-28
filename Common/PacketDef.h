#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>

enum class PacketType : uint16_t {
    None = 0,
    ChatMessage = 1,   // 일반 채팅 메시지
    Exit = 2,   // 종료 패킷
    UserCount = 3,    // 접속자 수 확인

    ChatBroadcast = 10,

    LoginReq = 11,
    LoginAck = 12,

    UserCountReq = 13,
    UserCountAck = 14,
    // 필요 시 계속 추가
};

#pragma pack(push, 1)
struct PacketHeader {
    uint16_t size;
    uint16_t id;    // PacketType 값

    PacketHeader() : size(0), id(0) {}
    PacketHeader(PacketType t, uint16_t s) : size(s), id(static_cast<uint16_t>(t)) {}
};
#pragma pack(pop)


// 메시지 구조체
// 내부/로그용
struct ChatMessageData {
    std::string sender;
    std::string message;
    uint64_t timestamp;
};

// 전송용
#pragma pack(push, 1)
struct ChatMessagePacket {
    PacketHeader header;
    char sender[32];
    char message[256];
    uint64_t timestamp;
};
#pragma pack(pop)


// 내부->전송용 변환
ChatMessagePacket ToPacket(const ChatMessageData& data) noexcept;
// 전송용 -> 내부 변환
ChatMessageData   ToData(const ChatMessagePacket& pkt);