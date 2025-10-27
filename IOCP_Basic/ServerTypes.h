#pragma once

#include <Utility.h>
#include "RingBuffer.h"

// AcceptEx 확장 함수 포인터 타입 정의
typedef BOOL(PASCAL* LPFN_ACCEPTEX)
(
    SOCKET sListenSocket, SOCKET sAcceptSocket,
    PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength,
    DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped
    );

// GetAcceptExSockaddrs 확장 함수 포인터 타입 정의
typedef VOID(PASCAL* LPFN_GETACCEPTEXSOCKADDRS)
(
    PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength,
    DWORD dwRemoteAddressLength, struct sockaddr** LocalSockaddr, LPINT LocalSockaddrLength,
    struct sockaddr** RemoteSockaddr, LPINT RemoteSockaddrLength
    );

// IOCP에서 사용할 확장 OVERLAPPED 구조체
// 어떤 작업(accept/recv/send)인지 구분하고, 세션 포인터와 버퍼를 포함
struct OverlappedEx : public OVERLAPPED {
    enum { OP_ACCEPT, OP_RECV, OP_SEND } op; // 작업 종류
    void* session;                           // 해당 세션 포인터(accept는 nullptr)
    WSABUF wsaBuf;                           // WSABUF 구조체(WSARecv/WSASend용)
    char buffer[NetConfig::BUFFER_SIZE];                // 실제 데이터 버퍼
    SOCKET acceptSock;                       // AcceptEx용 소켓
    OverlappedEx() : op(OP_ACCEPT), session(nullptr), acceptSock(INVALID_SOCKET) {
        ZeroMemory(this, sizeof(OVERLAPPED));
        wsaBuf.buf = buffer;
        wsaBuf.len = NetConfig::BUFFER_SIZE;
    }
};

// 클라이언트 1명당 관리되는 세션 구조체
struct Session {
    SOCKET sock;                             // 클라이언트 소켓
    char* rxBuf;                             // 수신 버퍼(버퍼 풀에서 임대)
    //size_t rxUsed;                           // 현재까지 받은 데이터 크기
    RingBuffer   rx;                         // 세션 고유 수신 링버퍼
    std::queue<std::vector<char>> sendQueue; // 송신 대기 큐(여러 패킷 병합 가능)
    OverlappedEx ovRecv, ovSend;             // 수신/송신용 OVERLAPPED 구조체
    std::atomic<bool> sending;               // 송신 중 여부(중복 송신 방지)
    std::atomic<bool> closing;               // 종료 처리 중 여부(이중 종료 방지)
    uint64_t id;                             // 세션 고유 ID(디버깅/모니터링용)
    Session() : sock(INVALID_SOCKET), rxBuf(nullptr), /*rxUsed(0),*/ rx(64 * 1024), sending(false), closing(false), id(0) {}
};
