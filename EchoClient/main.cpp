// SimpleTcpClient.cpp

#include "Utility.h"
#include "EchoClient.h"

int main() {

    EchoClient c;
    if (!c.init()) return 1;
    if (!c.createSocket()) return 1;
    if (!c.connectToServer()) return 1;
    c.run();                 // 내부에서 송신/수신 스레드 운용
    return 0;


    //// 1. WinSock 초기화
    //WSADATA wsaData;
    //if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
    //    std::cout << "WSAStartup 실패\n";
    //    return 1;
    //}

    //// 2. 소켓 생성
    //SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    //if (sock == INVALID_SOCKET) {
    //    std::cout << "소켓 생성 실패\n";
    //    WSACleanup();
    //    return 1;
    //}

    //// 3. 서버 주소 설정 (localhost:9000)
    //sockaddr_in serverAddr = {};
    //serverAddr.sin_family = AF_INET;
    //serverAddr.sin_port = htons(9000);
    //inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    //// 4. 서버에 연결
    //if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
    //    std::cout << "서버 연결 실패\n";
    //    closesocket(sock);
    //    WSACleanup();
    //    return 1;
    //}
    //std::cout << "서버에 연결 성공!\n";

    //// 5. 메시지 송수신 루프
    //while (true) {
    //    std::string msg;
    //    std::cout << "보낼 메시지(q 입력시 종료): ";
    //    std::getline(std::cin, msg);
    //    if (msg == "q" || msg == "Q") break;

    //    // 서버로 메시지 전송
    //    int sent = send(sock, msg.c_str(), (int)msg.size(), 0);
    //    if (sent <= 0) {
    //        std::cout << "send 실패\n";
    //        break;
    //    }

    //    // 서버로부터 에코 응답 수신
    //    char buf[4096] = {0};
    //    int received = recv(sock, buf, sizeof(buf)-1, 0);
    //    if (received <= 0) {
    //        std::cout << "서버 연결 종료 또는 recv 실패\n";
    //        break;
    //    }
    //    std::cout << "서버 응답: " << std::string(buf, received) << std::endl;
    //}

    //// 6. 종료 처리
    //closesocket(sock);
    //WSACleanup();
    //std::cout << "클라이언트 종료\n";
    //return 0;
}
