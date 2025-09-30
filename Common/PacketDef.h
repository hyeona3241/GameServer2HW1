#pragma once

#include <cstdint>
#include <string>

enum class PacketType : uint16_t {
    None = 0,
    ChatMessage = 1,   // �Ϲ� ä�� �޽���
    Exit = 2,   // ���� ��Ŷ
    UserCount = 3    // ������ �� Ȯ��
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
ChatMessagePacket ToPacket(const ChatMessageData & data) {
    ChatMessagePacket pkt{};
    pkt.header = PacketHeader(PacketType::ChatMessage, sizeof(ChatMessagePacket));

    strncpy(pkt.sender, data.sender.c_str(), sizeof(pkt.sender) - 1);
    strncpy(pkt.message, data.message.c_str(), sizeof(pkt.message) - 1);

    pkt.timestamp = data.timestamp;
    return pkt;
}

// ���ۿ� -> ���� ��ȯ
ChatMessageData ToData(const ChatMessagePacket& pkt) {
    ChatMessageData data;

    data.sender = pkt.sender;
    data.message = pkt.message;
    data.timestamp = pkt.timestamp;

    return data;
}