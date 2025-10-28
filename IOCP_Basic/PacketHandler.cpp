#include "PacketHandler.h"
#include <cstring>
#include <string>

static void ProcessOnePacket(PacketHandler * self, Session * s, const char* buf, size_t len) {
    if (len < sizeof(PacketHeader)) {
        Logger::Instance().Error("Invalid packet: too short (len=" + std::to_string(len) + ")");
        return;
    }

    PacketHeader h;
    std::memcpy(&h, buf, sizeof(PacketHeader));
    if (!PacketHandler::IsHeaderValid(h) || len < h.size) {
        Logger::Instance().Error("Invalid header or size mismatch (session=" + std::to_string(s->id) + ")");
        return;
    }

    const auto id = static_cast<PacketType>(h.id);

    switch (id) {
    case PacketType::LoginReq:
        if (h.size == sizeof(PacketHeader) + sizeof(PKLoginReqBody)) {
            self->HandleLogin(s, buf, len);
        }
        else {
            Logger::Instance().Error("Size mismatch for LoginReq. expected=" + std::to_string(sizeof(PacketHeader) + sizeof(PKLoginReqBody))
                + " got=" + std::to_string(h.size) + " (session=" + std::to_string(s->id) + ")");
        }
        break;

    case PacketType::ChatMessage:
        if (h.size == sizeof(ChatMessagePacket)) {
            ChatMessagePacket p{};
            std::memcpy(&p, buf, sizeof(p));
            self->HandleChatMessage(s, p);
        }
        else {
            Logger::Instance().Error("Size mismatch for ChatMessage packet. expected="
                + std::to_string(sizeof(ChatMessagePacket)) + " got=" + std::to_string(h.size)
                + " (session=" + std::to_string(s->id) + ")");
        }
        break;

    case PacketType::Exit:
        if (h.size == sizeof(PacketHeader) + sizeof(PKExitBody)) {
            self->HandleExit(s, buf, len);
        }
        else {
            Logger::Instance().Error("Size mismatch for Exit packet. expected=" + std::to_string(sizeof(PacketHeader) + sizeof(PKExitBody))
                + " got=" + std::to_string(h.size) + " (session=" + std::to_string(s->id) + ")");
        }
        break;

    case PacketType::UserCountReq:
        if (h.size == sizeof(PacketHeader)) {
            self->HandleUserCount(s);
        }
        else {
            Logger::Instance().Error("Size mismatch for UserCount packet. expected="
                + std::to_string(sizeof(PacketHeader)) + " got=" + std::to_string(h.size)
                + " (session=" + std::to_string(s->id) + ")");
        }
        break;

    default:
        Logger::Instance().Error("Unknown packet id=" + std::to_string(static_cast<int>(id))
            + " size=" + std::to_string(h.size) + " (session=" + std::to_string(s->id) + ")");
        break;
    }
}

void PacketHandler::OnClientConnected(Session* s) {
    Logger::Instance().Info("New connection: session=" + std::to_string(s->id));
    static const char kAskId[] = "[서버] 아이디를 입력하세요. 첫 번째 메시지가 아이디로 등록됩니다.\n";
    io_->EnqueueSend(s, kAskId, sizeof(kAskId) - 1);
    // TODO(LOG): 새 연결 로그 기록 (세션ID, 원격주소 등) -> 파일/CSV/JSON
}

void PacketHandler::OnBytes(Session* s, const char* buf, size_t len) {
    /*if (len < sizeof(PacketHeader)) {
        Logger::Instance().Error("Invalid packet: too short (len=" + std::to_string(len) + ")");
        return;
    }

    PacketHeader h;
    std::memcpy(&h, buf, sizeof(PacketHeader));
    if (!IsHeaderValid(h) || len < h.size) {
        Logger::Instance().Error("Invalid header or size mismatch (session=" + std::to_string(s->id) + ")");
        return;
    }

    const auto id = static_cast<PacketType>(h.id);

    switch (id) {
    case PacketType::ChatMessage:
        if (h.size == sizeof(ChatMessagePacket)) {
            ChatMessagePacket p{};
            std::memcpy(&p, buf, sizeof(p));
            HandleChatMessage(s, p);
        }
        else {
            Logger::Instance().Error("Size mismatch for ChatMessage packet. expected=" + std::to_string(sizeof(ChatMessagePacket))
                + " got=" + std::to_string(h.size) + " (session=" + std::to_string(s->id) + ")");
        }
        break;

    case PacketType::Exit:
        if (h.size == sizeof(PacketHeader)) {
            HandleExit(s);
        }
        else {
            Logger::Instance().Error("Size mismatch for Exit packet. expected=" + std::to_string(sizeof(PacketHeader))
                + " got=" + std::to_string(h.size) + " (session=" + std::to_string(s->id) + ")");
        }
        break;

    case PacketType::UserCount:
        if (h.size == sizeof(PacketHeader)) {
            HandleUserCount(s);
        }
        else {
            Logger::Instance().Error("Size mismatch for UserCount packet. expected=" + std::to_string(sizeof(PacketHeader))
                + " got=" + std::to_string(h.size) + " (session=" + std::to_string(s->id) + ")");
        }
        break;

    default:
        Logger::Instance().Error("Unknown packet id=" + std::to_string(static_cast<int>(id))
            + " size=" + std::to_string(h.size) + " (session=" + std::to_string(s->id) + ")");
        break;
    }*/

    ProcessOnePacket(this, s, buf, len);
}

void PacketHandler::OnPacket(Session* s, const std::vector<char>& packet) {
    if (packet.size() < sizeof(PacketHeader)) {
        Logger::Instance().Error("OnPacket: too short (session=" + std::to_string(s->id) + ")");
        return;
    }

    ProcessOnePacket(this, s, packet.data(), packet.size());
}

void PacketHandler::HandleChatMessage(Session* sender, const ChatMessagePacket& pkt) {
    ChatMessagePacket safe = pkt;
    safe.sender[sizeof(safe.sender) - 1] = '\0';
    safe.message[sizeof(safe.message) - 1] = '\0';

    ChatMessageData d = ToData(safe);
    Logger::Instance().Chat(d, sender->id);

    PKChatBroadcast bcast(safe.sender, safe.message, /*timestampMs*/ 0);
    const std::vector<char> bytes = bcast.Serialize();

    io_->Broadcast(bytes.data(), static_cast<int>(bytes.size()), sender);

}

void PacketHandler::HandleExit(Session* s, const char* buf, size_t len) {
    Logger::Instance().Info("Exit packet: session=" + std::to_string(s->id));

    if (len < sizeof(PacketHeader) + sizeof(PKExitBody)) {
        Logger::Instance().Warn("Exit packet too short (session=" + std::to_string(s->id) + ")");
        io_->CloseSession(s);
        return;
    }

    PKExit req;
    req.Deserialize(buf, len); // 유효성 검사 포함

    char name[32];
    std::strncpy(name, req.body.nickname, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';

    if (name[0] == '\0') {
        std::strncpy(name, "알수없음", sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
    }

    std::string notice = std::string(name) + " 님이 퇴장했습니다.";
    PKChatBroadcast bcast("서버", notice.c_str(), /*timestamp*/ 0);
    auto bytes = bcast.Serialize();
    io_->Broadcast(bytes.data(), static_cast<int>(bytes.size()), s);

    io_->CloseSession(s);
}

void PacketHandler::HandleUserCount(Session* s) {
    Logger::Instance().Info("UserCount request from session=" + std::to_string(s->id));

    const int32_t count = static_cast<int32_t>(io_->GetCurrentConnectionCount());

    PKUserCountAck ack(count);
    const std::vector<char> bytes = ack.Serialize();
    io_->EnqueueSend(s, bytes.data(), static_cast<int>(bytes.size()));
}

void PacketHandler::HandleLogin(Session* s, const char* buf, size_t len)
{
    PKLoginReq req;
    req.Deserialize(buf, len);

    const char* name = req.body.nickname;

    bool accepted = (name && name[0] != '\0');

    const int32_t cur = static_cast<int32_t>(io_->GetCurrentConnectionCount());

    PKLoginAck ack(s->id, accepted, accepted ? "" : "invalid nickname", cur);
    auto bytes = ack.Serialize();
    io_->EnqueueSend(s, bytes.data(), bytes.size());

    Logger::Instance().Info(std::string("LoginReq from session=") + std::to_string(s->id) + " nick='" + name + "' accepted=" + (accepted ? "1" : "0"));
}
