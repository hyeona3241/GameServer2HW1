#include "PacketHandler.h"
#include <cstring>
#include <string>

void PacketHandler::OnClientConnected(Session* s) {
    Logger::Instance().Info("New connection: session=" + std::to_string(s->id));
    static const char kAskId[] = "[서버] 아이디를 입력하세요. 첫 번째 메시지가 아이디로 등록됩니다.\n";
    io_->EnqueueSend(s, kAskId, sizeof(kAskId) - 1);
    // TODO(LOG): 새 연결 로그 기록 (세션ID, 원격주소 등) -> 파일/CSV/JSON
}

void PacketHandler::OnBytes(Session* s, const char* buf, size_t len) {
    if (len < sizeof(PacketHeader)) {
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
    }

}

void PacketHandler::HandleChatMessage(Session* sender, const ChatMessagePacket& pkt) {
    ChatMessagePacket safe = pkt;
    safe.sender[sizeof(safe.sender) - 1] = '\0';
    safe.message[sizeof(safe.message) - 1] = '\0';

    ChatMessageData d = ToData(safe);

    Logger::Instance().Chat(d, sender->id);

    std::string line;
    line.reserve(2 + std::strlen(safe.sender) + 1 + std::strlen(safe.message) + 2);
    line += "[";
    line += safe.sender;
    line += "] ";
    line += safe.message;
    line += "\n";

    io_->Broadcast(line.c_str(), line.size(), sender);

}

void PacketHandler::HandleExit(Session* s) {
    Logger::Instance().Info("Exit packet: session=" + std::to_string(s->id));
    io_->CloseSession(s);
}

void PacketHandler::HandleUserCount(Session* s) {
    Logger::Instance().Info("UserCount request from session=" + std::to_string(s->id));

    const int count = io_->GetCurrentConnectionCount();

    std::string msg = "[서버] 현재 접속자 수: " + std::to_string(count) + "\n";
    io_->EnqueueSend(s, msg.c_str(), msg.size());

}