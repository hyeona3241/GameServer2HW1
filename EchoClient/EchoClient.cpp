#include "EchoClient.h"

// main�� 1�� WinSock �ʱ�ȭ
bool EchoClient::init()
{
    if (WSAStartup(MAKEWORD(2, 2), &wsaData_) != 0) {
        std::cout << "WSAStartup ����\n";
        return false;
    }
    return true;
}

// main�� 2�� ���� ����
bool EchoClient::createSocket()
{
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == INVALID_SOCKET) {
        std::cout << "���� ���� ����\n";
        WSACleanup();
        return false;
    }
    return true;
}

bool EchoClient::connectToServer()
{
    // main�� 3�� ���� �ּ� ���� (localhost:9000)
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
        std::cout << "inet_pton ����\n";
        closesocket(sock_);
        WSACleanup();
        return false;
    }

    // main�� 4�� ������ ����
    if (connect(sock_, (sockaddr*)&serverAddr_, sizeof(serverAddr_)) == SOCKET_ERROR) {
        std::cout << "���� ���� ����\n";
        closesocket(sock_);
        WSACleanup();
        return false;
    }

    std::cout << "������ ���� ����!\n";
    return true;
}

// main 5�� �޽��� �ۼ��� ���� (��Ƽ ������� �и�)
void EchoClient::run()
{
    running_ = true;

    std::thread rx([this] { receiveLoop(); });
    std::thread tx([this] { sendLoop(); });

    // �� �� ���� ������ ���
    if (tx.joinable()) tx.join();
    if (rx.joinable()) rx.join();

    // �ڿ� ����
    stop();
}

// main�� 6�� ���� ó��
void EchoClient::stop()
{
    bool expected = true;
    if (running_.compare_exchange_strong(expected, false)) {
        // ���ŷ recv()�� ������ receiveLoop�� ����ǵ���
        if (sock_ != INVALID_SOCKET) {
            shutdown(sock_, SD_BOTH);
            closesocket(sock_);
            sock_ = INVALID_SOCKET;
        }
        WSACleanup();
        std::cout << "Ŭ���̾�Ʈ ����\n";
    }
}

// �ۼ��� ����
void EchoClient::receiveLoop()
{
    while (running_) {
        int received = recv(sock_, rxBuf_, sizeof(rxBuf_) - 1, 0);
        if (received <= 0) {
            std::cout << "���� ���� ���� �Ǵ� recv ����\n";
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
                        std::cout << "[����] �α��� ����! ����ID=" << ack.body.sessionId << " ���� ������ ��=" << ack.body.currentUsers << "\n";
                        std::cout << "[�˸�] ���̵� '" << nickname_ << "'(��)�� �����Ǿ����ϴ�.\n\n";
                    }
                    else
                        std::cout << "[����] �α��� ����: " << ack.body.reason << "\n";
                }
                else {
                    std::cout << "[���] �߸��� LoginAck ��Ŷ ����(" << header.size << "B)\n";
                }
                break;
            
            case PacketType::ChatBroadcast: 
                if (received >= header.size && header.size == sizeof(PacketHeader) + sizeof(PKChatBroadcastBody)) {
                    PKChatBroadcast b;
                    b.Deserialize(rxBuf_, received);
                    std::cout << "[" << b.body.sender << "] " << b.body.message << "\n";
                }
                else {
                    std::cout << "[���] �߸��� ChatBroadcast ��Ŷ(" << header.size << "B)\n";
                }
                break;

            case PacketType::UserCountAck:
                if (received >= header.size && header.size == sizeof(PacketHeader) + sizeof(int32_t)) {
                    PKUserCountAck ack;
                    ack.Deserialize(rxBuf_, received);
                    std::cout << "[����] ���� ������ ��: " << ack.body.count << "\n";
                }
                else {
                    std::cout << "[���] �߸��� UserCountAck ��Ŷ ���� (" << header.size << "����Ʈ)\n";
                }
                break;

            default:
                rxBuf_[received] = '\0';
                std::cout << rxBuf_;
                break;
            }
        }
        else {
            // ��Ŷ ��� ������ ���ڿ� ���
            rxBuf_[received] = '\0';
            std::cout << rxBuf_;
        }
    }
    // ������ ������ ��ü ����
    stop();
}

void EchoClient::sendLoop()
{ 
    std::cout << "�޽����� ���� �� �ֽ��ϴ� (/������, /�����ڼ�, /�����α�)\n";

    while (running_) {
        std::string line;
        if (!std::getline(std::cin, line)) break;

        if (nickname_.empty() && line != "/������" && line != "/�����ڼ�" && line != "/�����α�") {
            nickname_ = line;
            PKLoginReq req(nickname_.c_str());
            if (!sendPacket(req)) std::cout << "LoginReq ���� ����\n";

            continue;
        }

        if (line == "/������") {
            PKExit pk(nickname_.c_str());          // �г��� ����
            auto bytes = pk.Serialize();

            if (!sendPacket(pk)) std::cout << "Exit ��Ŷ ���� ����\n";
            break;
        }
        else if (line == "/�����ڼ�") {
            PKUserCountReq pk;
            if (!sendPacket(pk)) std::cout << "UserCount ��Ŷ ���� ����\n";
            continue;
        }
        else if (line == "/�����α�") {
            struct FakePacket {
                PacketHeader header;
                char dummy[4];
            } fake{};

            fake.header = PacketHeader(PacketType::ChatMessage,
                static_cast<uint16_t>(9999)); // �߸��� ũ�� ������ üũ

            sendBytesAll(reinterpret_cast<char*>(&fake), sizeof(fake));
            std::cout << "[�׽�Ʈ] �߸��� ��Ŷ�� �����߽��ϴ�.\n";
            continue;
        }
        else {
            ChatMessageData data;
            data.sender = nickname_.empty() ? "Unknown" : nickname_;
            data.message = line;
            data.timestamp = nowUnixMillis();

            PKChatMessage pk(data);
            if (!sendPacket(pk)) {
                std::cout << "ChatMessage ��Ŷ ���� ����\n";
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
