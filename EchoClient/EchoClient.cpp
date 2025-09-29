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
        rxBuf_[received] = '\0';
        std::cout << "서버 응답: " << rxBuf_ << std::endl;
    }
    // 수신이 끝나면 전체 종료
    stop();
}

void EchoClient::sendLoop()
{
    while (running_) {
        std::string msg;
        std::cout << "보낼 메시지(q 입력시 종료): ";
        if (!std::getline(std::cin, msg)) {
            break;
        }
        if (msg == "q" || msg == "Q") {
            break;
        }
        int sent = send(sock_, msg.c_str(), (int)msg.size(), 0);
        if (sent <= 0) {
            std::cout << "send 실패\n";
            break;
        }
    }
    // 송신 루프 종료 시 전체 종료
    stop();
}
