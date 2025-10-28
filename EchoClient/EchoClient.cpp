#include "EchoClient.h"

// main의 1번 WinSock 초기화
bool EchoClient::init()
{
    if (WSAStartup(MAKEWORD(2, 2), &wsaData_) != 0) {
        std::cout << "WSAStartup 실패\n";
        return false;
    }
    return true;
}

// main의 2번 소켓 생성
bool EchoClient::createSocket()
{
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == INVALID_SOCKET) {
        std::cout << "소켓 생성 실패\n";
        WSACleanup();
        return false;
    }
    return true;
}

bool EchoClient::connectToServer()
{
    // main의 3번 서버 주소 설정 (localhost:9000)
    serverAddr_ = {};
    serverAddr_.sin_family = AF_INET;

#ifdef NetConfig_SERVER_PORT_EXISTS
    serverAddr_.sin_port = htons(NetConfig::SERVER_PORT);
#else
    serverAddr_.sin_port = htons(9000);
#endif

#ifdef NetConfig_SERVER_IP_EXISTS
    if (inet_pton(AF_INET, NetConfig::SERVER_IP, &serverAddr_.sin_addr) != 1)
#else
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr_.sin_addr) != 1) 
#endif
    {
        std::cout << "inet_pton 실패\n";
        closesocket(sock_);
        WSACleanup();
        return false;
    }

    // main의 4번 서버에 연결
    if (connect(sock_, (sockaddr*)&serverAddr_, sizeof(serverAddr_)) == SOCKET_ERROR) {
        std::cout << "서버 연결 실패\n";
        closesocket(sock_);
        WSACleanup();
        return false;
    }

    std::cout << "서버에 연결 성공!\n";
    return true;
}

// main 5의 메시지 송수신 루프 (멀티 스레드로 분리)
void EchoClient::run()
{
    running_ = true;

    std::thread rx([this] { receiveLoop(); });
    std::thread tx([this] { sendLoop(); });

    // 둘 다 끝날 때까지 대기
    if (tx.joinable()) tx.join();
    if (rx.joinable()) rx.join();

    // 자원 정리
    stop();
}

// main의 6번 종료 처리
void EchoClient::stop()
{
    bool expected = true;
    if (running_.compare_exchange_strong(expected, false)) {
        // 블로킹 recv()를 깨워서 receiveLoop가 종료되도록
        if (sock_ != INVALID_SOCKET) {
            shutdown(sock_, SD_BOTH);
            closesocket(sock_);
            sock_ = INVALID_SOCKET;
        }
        WSACleanup();
        std::cout << "클라이언트 종료\n";
    }
}

// 송수신 루프
void EchoClient::receiveLoop()
{
    while (running_) {
        int received = recv(sock_, rxBuf_, sizeof(rxBuf_) - 1, 0);
        if (received <= 0) {
            std::cout << "서버 연결 종료 또는 recv 실패\n";
            break;
        }
        if (received >= sizeof(PacketHeader)) {
            PacketHeader header;
            std::memcpy(&header, rxBuf_, sizeof(PacketHeader));

            switch (static_cast<PacketType>(header.id)) {
            case PacketType::LoginAck:
                if (received >= header.size && header.size == sizeof(PacketHeader) + sizeof(PKLoginAckBody)) {
                    PKLoginAck ack;
                    ack.Deserialize(rxBuf_, received);

                    if (ack.body.accepted) {
                        std::cout << "[서버] 로그인 성공! 세션ID=" << ack.body.sessionId << " 현재 접속자 수=" << ack.body.currentUsers << "\n";
                        std::cout << "[알림] 아이디가 '" << nickname_ << "'(으)로 설정되었습니다.\n\n";
                    }
                    else
                        std::cout << "[서버] 로그인 실패: " << ack.body.reason << "\n";
                }
                else {
                    std::cout << "[경고] 잘못된 LoginAck 패킷 수신(" << header.size << "B)\n";
                }
                break;
            
            case PacketType::ChatBroadcast: 
                if (received >= header.size && header.size == sizeof(PacketHeader) + sizeof(PKChatBroadcastBody)) {
                    PKChatBroadcast b;
                    b.Deserialize(rxBuf_, received);
                    std::cout << "[" << b.body.sender << "] " << b.body.message << "\n";
                }
                else {
                    std::cout << "[경고] 잘못된 ChatBroadcast 패킷(" << header.size << "B)\n";
                }
                break;

            case PacketType::UserCountAck:
                if (received >= header.size && header.size == sizeof(PacketHeader) + sizeof(int32_t)) {
                    PKUserCountAck ack;
                    ack.Deserialize(rxBuf_, received);
                    std::cout << "[서버] 현재 접속자 수: " << ack.body.count << "\n";
                }
                else {
                    std::cout << "[경고] 잘못된 UserCountAck 패킷 수신 (" << header.size << "바이트)\n";
                }
                break;

            default:
                rxBuf_[received] = '\0';
                std::cout << rxBuf_;
                break;
            }
        }
        else {
            // 패킷 헤더 없으면 문자열 취급
            rxBuf_[received] = '\0';
            std::cout << rxBuf_;
        }
    }
    // 수신이 끝나면 전체 종료
    stop();
}

void EchoClient::sendLoop()
{ 
    std::cout << "메시지를 보낼 수 있습니다 (/나가기, /접속자수, /에러로그)\n";

    while (running_) {
        std::string line;
        if (!std::getline(std::cin, line)) break;

        if (nickname_.empty() && line != "/나가기" && line != "/접속자수" && line != "/에러로그") {
            nickname_ = line;
            PKLoginReq req(nickname_.c_str());
            if (!sendPacket(req)) std::cout << "LoginReq 전송 실패\n";

            continue;
        }

        if (line == "/나가기") {
            PKExit pk(nickname_.c_str());          // 닉네임 포함
            auto bytes = pk.Serialize();

            if (!sendPacket(pk)) std::cout << "Exit 패킷 전송 실패\n";
            break;
        }
        else if (line == "/접속자수") {
            PKUserCountReq pk;
            if (!sendPacket(pk)) std::cout << "UserCount 패킷 전송 실패\n";
            continue;
        }
        else if (line == "/에러로그") {
            struct FakePacket {
                PacketHeader header;
                char dummy[4];
            } fake{};

            fake.header = PacketHeader(PacketType::ChatMessage,
                static_cast<uint16_t>(9999)); // 잘못된 크기 보내서 체크

            sendBytesAll(reinterpret_cast<char*>(&fake), sizeof(fake));
            std::cout << "[테스트] 잘못된 패킷을 전송했습니다.\n";
            continue;
        }
        else {
            ChatMessageData data;
            data.sender = nickname_.empty() ? "Unknown" : nickname_;
            data.message = line;
            data.timestamp = nowUnixMillis();

            PKChatMessage pk(data);
            if (!sendPacket(pk)) {
                std::cout << "ChatMessage 패킷 전송 실패\n";
                break;
            }
        }
    }
    stop();
}

bool EchoClient::sendPacket(const IPacket& pkt) {
    auto bytes = pkt.Serialize();

    return sendBytesAll(bytes.data(), bytes.size());
}

bool EchoClient::sendBytesAll(const char* data, size_t len) {
    size_t sentTotal = 0;

    while (sentTotal < len) {
        int s = ::send(sock_, data + sentTotal, (int)(len - sentTotal), 0);
        if (s <= 0) return false;
        sentTotal += s;
    }

    return true;
}

uint64_t EchoClient::nowUnixMillis() {
    using namespace std::chrono;

    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
