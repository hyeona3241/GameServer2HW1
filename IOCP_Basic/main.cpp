// main.cpp
#include "IOCP_EchoServer.h"

int main() {
    IOCP_EchoServer server(9000);
    if (server.Start()) {
        std::cout << "������ ���۵Ǿ����ϴ�. �����Ϸ��� q �Է� �� ����.\n";
        char c;
        while (std::cin >> c) {
            if (c == 'q' || c == 'Q') break;
        }
        server.Stop();
    }
    else {
        std::cout << "���� ���� ����\n";
    }
    return 0;
}
