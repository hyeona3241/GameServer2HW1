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
        rxBuf_[received] = '\0';
        std::cout << "���� ����: " << rxBuf_ << std::endl;
    }
    // ������ ������ ��ü ����
    stop();
}

void EchoClient::sendLoop()
{
    while (running_) {
        std::string msg;
        std::cout << "���� �޽���(q �Է½� ����): ";
        if (!std::getline(std::cin, msg)) {
            break;
        }
        if (msg == "q" || msg == "Q") {
            break;
        }
        int sent = send(sock_, msg.c_str(), (int)msg.size(), 0);
        if (sent <= 0) {
            std::cout << "send ����\n";
            break;
        }
    }
    // �۽� ���� ���� �� ��ü ����
    stop();
}
