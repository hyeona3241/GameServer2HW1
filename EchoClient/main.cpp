// SimpleTcpClient.cpp

#include "Utility.h"
#include "EchoClient.h"

int main() {

    EchoClient c;
    if (!c.init()) return 1;
    if (!c.createSocket()) return 1;
    if (!c.connectToServer()) return 1;
    c.run();                 // ���ο��� �۽�/���� ������ ���
    return 0;


    //// 1. WinSock �ʱ�ȭ
    //WSADATA wsaData;
    //if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
    //    std::cout << "WSAStartup ����\n";
    //    return 1;
    //}

    //// 2. ���� ����
    //SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    //if (sock == INVALID_SOCKET) {
    //    std::cout << "���� ���� ����\n";
    //    WSACleanup();
    //    return 1;
    //}

    //// 3. ���� �ּ� ���� (localhost:9000)
    //sockaddr_in serverAddr = {};
    //serverAddr.sin_family = AF_INET;
    //serverAddr.sin_port = htons(9000);
    //inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    //// 4. ������ ����
    //if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
    //    std::cout << "���� ���� ����\n";
    //    closesocket(sock);
    //    WSACleanup();
    //    return 1;
    //}
    //std::cout << "������ ���� ����!\n";

    //// 5. �޽��� �ۼ��� ����
    //while (true) {
    //    std::string msg;
    //    std::cout << "���� �޽���(q �Է½� ����): ";
    //    std::getline(std::cin, msg);
    //    if (msg == "q" || msg == "Q") break;

    //    // ������ �޽��� ����
    //    int sent = send(sock, msg.c_str(), (int)msg.size(), 0);
    //    if (sent <= 0) {
    //        std::cout << "send ����\n";
    //        break;
    //    }

    //    // �����κ��� ���� ���� ����
    //    char buf[4096] = {0};
    //    int received = recv(sock, buf, sizeof(buf)-1, 0);
    //    if (received <= 0) {
    //        std::cout << "���� ���� ���� �Ǵ� recv ����\n";
    //        break;
    //    }
    //    std::cout << "���� ����: " << std::string(buf, received) << std::endl;
    //}

    //// 6. ���� ó��
    //closesocket(sock);
    //WSACleanup();
    //std::cout << "Ŭ���̾�Ʈ ����\n";
    //return 0;
}
