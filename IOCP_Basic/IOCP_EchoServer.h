// IOCP_EchoServer.h
#pragma once

#include <Utility.h>
#include "ServerTypes.h"

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
