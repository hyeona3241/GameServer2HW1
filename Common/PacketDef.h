#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>

enum class PacketType : uint16_t {
    None = 0,
    ChatMessage = 1,   // �Ϲ� ä�� �޽���
    Exit = 2,   // ���� ��Ŷ
    UserCount = 3,    // ������ �� Ȯ��

    ChatBroadcast = 10,

    LoginReq = 11,
    LoginAck = 12,

    UserCountReq = 13,
    UserCountAck = 14,
    // �ʿ� �� ��� �߰�
};

#pragma pack(push, 1)
struct PacketHeader {
    uint16_t size;
    uint16_t id;    // PacketType ��

    PacketHeader() : size(0), id(0) {}
    PacketHeader(PacketType t, uint16_t s) : size(s), id(static_cast<uint16_t>(t)) {}
};
#pragma pack(pop)


// �޽��� ����ü
// ����/�α׿�
struct ChatMessageData {
    std::string sender;
    std::string message;
    uint64_t timestamp;
};

// ���ۿ�
#pragma pack(push, 1)
struct ChatMessagePacket {
    PacketHeader header;
    char sender[32];
    char message[256];
    uint64_t timestamp;
};
#pragma pack(pop)


// ����->���ۿ� ��ȯ
ChatMessagePacket ToPacket(const ChatMessageData& data) noexcept;
// ���ۿ� -> ���� ��ȯ
ChatMessageData   ToData(const ChatMessagePacket& pkt);