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

    // 새 연결 직후 호출(“아이디를 입력하세요” 안내 전송)
    void OnClientConnected(Session* s);

    // 수신 바이트 도착 시 호출 (buf: 이번에 막 도착한 바이트)
    // NOTE: 지금은 단순화. 추후 프레이머로 다중/부분 패킷 조립 예정.
    void OnBytes(Session* s, const char* buf, size_t len);

    void OnPacket(Session* s, const std::vector<char>& packet);

public:
    // 개별 패킷 처리
    void HandleChatMessage(Session* s, const ChatMessagePacket& pkt);
    void HandleExit(Session* s);
    void HandleUserCount(Session* s);

    // 단순 검증
    static bool IsHeaderValid(const PacketHeader& h) {
        return h.size >= sizeof(PacketHeader);
    }

};
