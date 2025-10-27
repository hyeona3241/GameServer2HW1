#pragma once

#include <Utility.h>
#include "RingBuffer.h"

// AcceptEx Ȯ�� �Լ� ������ Ÿ�� ����
typedef BOOL(PASCAL* LPFN_ACCEPTEX)
(
    SOCKET sListenSocket, SOCKET sAcceptSocket,
    PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength,
    DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped
    );

// GetAcceptExSockaddrs Ȯ�� �Լ� ������ Ÿ�� ����
typedef VOID(PASCAL* LPFN_GETACCEPTEXSOCKADDRS)
(
    PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength,
    DWORD dwRemoteAddressLength, struct sockaddr** LocalSockaddr, LPINT LocalSockaddrLength,
    struct sockaddr** RemoteSockaddr, LPINT RemoteSockaddrLength
    );

// IOCP���� ����� Ȯ�� OVERLAPPED ����ü
// � �۾�(accept/recv/send)���� �����ϰ�, ���� �����Ϳ� ���۸� ����
struct OverlappedEx : public OVERLAPPED {
    enum { OP_ACCEPT, OP_RECV, OP_SEND } op; // �۾� ����
    void* session;                           // �ش� ���� ������(accept�� nullptr)
    WSABUF wsaBuf;                           // WSABUF ����ü(WSARecv/WSASend��)
    char buffer[NetConfig::BUFFER_SIZE];                // ���� ������ ����
    SOCKET acceptSock;                       // AcceptEx�� ����
    OverlappedEx() : op(OP_ACCEPT), session(nullptr), acceptSock(INVALID_SOCKET) {
        ZeroMemory(this, sizeof(OVERLAPPED));
        wsaBuf.buf = buffer;
        wsaBuf.len = NetConfig::BUFFER_SIZE;
    }
};

// Ŭ���̾�Ʈ 1��� �����Ǵ� ���� ����ü
struct Session {
    SOCKET sock;                             // Ŭ���̾�Ʈ ����
    char* rxBuf;                             // ���� ����(���� Ǯ���� �Ӵ�)
    //size_t rxUsed;                           // ������� ���� ������ ũ��
    RingBuffer   rx;                         // ���� ���� ���� ������
    std::queue<std::vector<char>> sendQueue; // �۽� ��� ť(���� ��Ŷ ���� ����)
    OverlappedEx ovRecv, ovSend;             // ����/�۽ſ� OVERLAPPED ����ü
    std::atomic<bool> sending;               // �۽� �� ����(�ߺ� �۽� ����)
    std::atomic<bool> closing;               // ���� ó�� �� ����(���� ���� ����)
    uint64_t id;                             // ���� ���� ID(�����/����͸���)
    Session() : sock(INVALID_SOCKET), rxBuf(nullptr), /*rxUsed(0),*/ rx(64 * 1024), sending(false), closing(false), id(0) {}
};
