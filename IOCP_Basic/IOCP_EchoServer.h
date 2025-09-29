// IOCP_EchoServer.h
#pragma once
#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <thread>
#include <iostream>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

// �� ���� ����� ���� ũ��(4KB)
constexpr int BUFFER_SIZE = 4096;
// ���ÿ� �̸� �޾Ƶ� AcceptEx ����(�⺻ 16, �ھ� ���� ���� ���� ����)
constexpr int DEFAULT_ACCEPT = 16;

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
    char buffer[BUFFER_SIZE];                // ���� ������ ����
    SOCKET acceptSock;                       // AcceptEx�� ����
    OverlappedEx() : op(OP_ACCEPT), session(nullptr), acceptSock(INVALID_SOCKET) {
        ZeroMemory(this, sizeof(OVERLAPPED));
        wsaBuf.buf = buffer;
        wsaBuf.len = BUFFER_SIZE;
    }
};

// Ŭ���̾�Ʈ 1��� �����Ǵ� ���� ����ü
struct Session {
    SOCKET sock;                             // Ŭ���̾�Ʈ ����
    char* rxBuf;                             // ���� ����(���� Ǯ���� �Ӵ�)
    size_t rxUsed;                           // ������� ���� ������ ũ��
    std::queue<std::vector<char>> sendQueue; // �۽� ��� ť(���� ��Ŷ ���� ����)
    OverlappedEx ovRecv, ovSend;             // ����/�۽ſ� OVERLAPPED ����ü
    std::atomic<bool> sending;               // �۽� �� ����(�ߺ� �۽� ����)
    std::atomic<bool> closing;               // ���� ó�� �� ����(���� ���� ����)
    uint64_t id;                             // ���� ���� ID(�����/����͸���)
    Session() : sock(INVALID_SOCKET), rxBuf(nullptr), rxUsed(0), sending(false), closing(false), id(0) {}
};

// ���� ũ�� ���� Ǯ(���� ����ȭ)
// ������ ���� ���۸� �� ��� Ǯ���� �Ӵ�/�ݳ�
class BufferPool {
public:
    BufferPool(size_t blockSize, size_t initialCount);
    ~BufferPool();
    char* Allocate();    // ���� �Ӵ�
    void Release(char* buf); // ���� �ݳ�
private:
    std::mutex mtx_;             // ���� ���� ��ȣ�� ���ؽ�
    std::vector<char*> pool_;    // ���� ������ ����Ʈ
    size_t blockSize_;           // ���� ũ��
};

// IOCP ��� ���� ���� Ŭ����
class IOCP_EchoServer {
public:
    // ��Ʈ, ��Ŀ ������ ��, AcceptEx ���� ����(0�̸� �ڵ� ����)
    IOCP_EchoServer(unsigned short port, int workerCount = 0, int acceptCount = 0);
    ~IOCP_EchoServer();
    bool Start();    // ���� ����
    void Stop();     // ���� ���� �� ���ҽ� ����
private:
    // Ȯ�� �Լ�(WSAIoctl�� AcceptEx ��) �ε�
    void LoadExtensionFunctions();
    // AcceptEx�� �̸� ���(���Խ�)
    void PostAccept();
    // AcceptEx �Ϸ� ó��(�� ���� ����, IOCP ��� ��)
    void AcceptCompletion(OverlappedEx* ov, DWORD bytes);
    // WSARecv �Ϸ� ó��(���� ������ ó��, ���� �۽� ��)
    void RecvCompletion(Session* session, DWORD bytes);
    // WSASend �Ϸ� ó��(�۽� ť�� ���� ������ ������ ��۽�)
    void SendCompletion(Session* session, DWORD bytes);
    // ���� ���� �� ���ҽ� ��ȯ(���� ���� ����)
    void CloseSession(Session* session);
    // IOCP ��Ŀ ������ �Լ�(��� I/O �Ϸ� ó��)
    void WorkerThread();
    // ��� ��� ������(1�ʸ��� ���� ���)
    void StatThread();
    // ���� ��û ���(WSARecv)
    void PostRecv(Session* session);
    // �۽� ť�� ������ �߰� �� �ʿ�� �۽� ����
    void EnqueueSend(Session* session, const char* data, size_t len);
    // ���� �۽� ��û(WSASend)
    void PostSend(Session* session);

    SOCKET listenSocket_;                    // ���� ����
    HANDLE iocpHandle_;                      // IOCP �ڵ�
    std::vector<std::thread> workerThreads_; // ��Ŀ ������ ���
    std::thread statThread_;                 // ��� ������
    std::atomic<bool> running_;              // ���� ���� �� ����
    unsigned short port_;                    // ���� ��Ʈ

    // Ȯ�� �Լ� ������(AcceptEx, GetAcceptExSockaddrs)
    LPFN_ACCEPTEX fnAcceptEx_;
    LPFN_GETACCEPTEXSOCKADDRS fnGetAcceptExSockaddrs_;

    BufferPool bufferPool_;                  // ���� Ǯ(���� ���ۿ�)
    std::unordered_map<uint64_t, Session*> sessionTable_; // ���� ���̺�(ID��������)
    std::mutex sessionMtx_;                  // ���� ���̺� ��ȣ�� ���ؽ�
    std::atomic<uint64_t> sessionIdGen_;     // ���� ID �߱ޱ�
    int acceptOutstanding_;                  // ���ÿ� ����� AcceptEx ����
    int workerCount_;                        // ��Ŀ ������ ��

    // ��� ����(���� ��, �ʴ� accept/recv/send)
    std::atomic<int> statCurConn_;
    std::atomic<int> statAccept_;
    std::atomic<int> statRecv_;
    std::atomic<int> statSend_;
};
