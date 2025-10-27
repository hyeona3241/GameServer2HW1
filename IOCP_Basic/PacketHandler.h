#pragma once

#include <cstdint>
#include <vector>
//#include <unordered_map>
#include "PacketDef.h"
#include "ServerTypes.h"
#include "Logger.h"


struct ISendClose {
    virtual void EnqueueSend(Session* s, const char* data, size_t len) = 0;
    virtual void CloseSession(Session* s) = 0;
    virtual int  GetCurrentConnectionCount() const = 0;
    virtual void Broadcast(const char* data, size_t len, Session* exclude) = 0;

    virtual ~ISendClose() = default;
};

class PacketHandler
{
private:
    ISendClose* io_;

public:
    explicit PacketHandler(ISendClose* io) : io_(io) {}

    // �� ���� ���� ȣ��(�����̵� �Է��ϼ��䡱 �ȳ� ����)
    void OnClientConnected(Session* s);

    // ���� ����Ʈ ���� �� ȣ�� (buf: �̹��� �� ������ ����Ʈ)
    // NOTE: ������ �ܼ�ȭ. ���� �����̸ӷ� ����/�κ� ��Ŷ ���� ����.
    void OnBytes(Session* s, const char* buf, size_t len);

    void OnPacket(Session* s, const std::vector<char>& packet);

public:
    // ���� ��Ŷ ó��
    void HandleChatMessage(Session* s, const ChatMessagePacket& pkt);
    void HandleExit(Session* s);
    void HandleUserCount(Session* s);

    // �ܼ� ����
    static bool IsHeaderValid(const PacketHeader& h) {
        return h.size >= sizeof(PacketHeader);
    }

};
