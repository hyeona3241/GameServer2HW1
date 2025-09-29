// main.cpp
#include "IOCP_EchoServer.h"

int main() {
    IOCP_EchoServer server(9000);
    if (server.Start()) {
        std::cout << "서버가 시작되었습니다. 종료하려면 q 입력 후 엔터.\n";
        char c;
        while (std::cin >> c) {
            if (c == 'q' || c == 'Q') break;
        }
        server.Stop();
    }
    else {
        std::cout << "서버 시작 실패\n";
    }
    return 0;
}
