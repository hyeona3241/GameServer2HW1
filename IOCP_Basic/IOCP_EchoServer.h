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

// 한 번에 사용할 버퍼 크기(4KB)
constexpr int BUFFER_SIZE = 4096;
// 동시에 미리 받아둘 AcceptEx 개수(기본 16, 코어 수에 따라 조정 가능)
constexpr int DEFAULT_ACCEPT = 16;

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
    char buffer[BUFFER_SIZE];                // 실제 데이터 버퍼
    SOCKET acceptSock;                       // AcceptEx용 소켓
    OverlappedEx() : op(OP_ACCEPT), session(nullptr), acceptSock(INVALID_SOCKET) {
        ZeroMemory(this, sizeof(OVERLAPPED));
        wsaBuf.buf = buffer;
        wsaBuf.len = BUFFER_SIZE;
    }
};

// 클라이언트 1명당 관리되는 세션 구조체
struct Session {
    SOCKET sock;                             // 클라이언트 소켓
    char* rxBuf;                             // 수신 버퍼(버퍼 풀에서 임대)
    size_t rxUsed;                           // 현재까지 받은 데이터 크기
    std::queue<std::vector<char>> sendQueue; // 송신 대기 큐(여러 패킷 병합 가능)
    OverlappedEx ovRecv, ovSend;             // 수신/송신용 OVERLAPPED 구조체
    std::atomic<bool> sending;               // 송신 중 여부(중복 송신 방지)
    std::atomic<bool> closing;               // 종료 처리 중 여부(이중 종료 방지)
    uint64_t id;                             // 세션 고유 ID(디버깅/모니터링용)
    Session() : sock(INVALID_SOCKET), rxBuf(nullptr), rxUsed(0), sending(false), closing(false), id(0) {}
};

// 고정 크기 버퍼 풀(성능 최적화)
// 세션의 수신 버퍼를 힙 대신 풀에서 임대/반납
class BufferPool {
public:
    BufferPool(size_t blockSize, size_t initialCount);
    ~BufferPool();
    char* Allocate();    // 버퍼 임대
    void Release(char* buf); // 버퍼 반납
private:
    std::mutex mtx_;             // 동시 접근 보호용 뮤텍스
    std::vector<char*> pool_;    // 버퍼 포인터 리스트
    size_t blockSize_;           // 버퍼 크기
};

// IOCP 기반 에코 서버 클래스
class IOCP_EchoServer {
public:
    // 포트, 워커 스레드 수, AcceptEx 개수 지정(0이면 자동 결정)
    IOCP_EchoServer(unsigned short port, int workerCount = 0, int acceptCount = 0);
    ~IOCP_EchoServer();
    bool Start();    // 서버 시작
    void Stop();     // 서버 정지 및 리소스 해제
private:
    // 확장 함수(WSAIoctl로 AcceptEx 등) 로딩
    void LoadExtensionFunctions();
    // AcceptEx를 미리 등록(선게시)
    void PostAccept();
    // AcceptEx 완료 처리(새 세션 생성, IOCP 등록 등)
    void AcceptCompletion(OverlappedEx* ov, DWORD bytes);
    // WSARecv 완료 처리(수신 데이터 처리, 에코 송신 등)
    void RecvCompletion(Session* session, DWORD bytes);
    // WSASend 완료 처리(송신 큐에 남은 데이터 있으면 재송신)
    void SendCompletion(Session* session, DWORD bytes);
    // 세션 종료 및 리소스 반환(이중 종료 방지)
    void CloseSession(Session* session);
    // IOCP 워커 스레드 함수(모든 I/O 완료 처리)
    void WorkerThread();
    // 통계 출력 스레드(1초마다 상태 출력)
    void StatThread();
    // 수신 요청 등록(WSARecv)
    void PostRecv(Session* session);
    // 송신 큐에 데이터 추가 및 필요시 송신 시작
    void EnqueueSend(Session* session, const char* data, size_t len);
    // 실제 송신 요청(WSASend)
    void PostSend(Session* session);

    SOCKET listenSocket_;                    // 리슨 소켓
    HANDLE iocpHandle_;                      // IOCP 핸들
    std::vector<std::thread> workerThreads_; // 워커 스레드 목록
    std::thread statThread_;                 // 통계 스레드
    std::atomic<bool> running_;              // 서버 실행 중 여부
    unsigned short port_;                    // 리슨 포트

    // 확장 함수 포인터(AcceptEx, GetAcceptExSockaddrs)
    LPFN_ACCEPTEX fnAcceptEx_;
    LPFN_GETACCEPTEXSOCKADDRS fnGetAcceptExSockaddrs_;

    BufferPool bufferPool_;                  // 버퍼 풀(수신 버퍼용)
    std::unordered_map<uint64_t, Session*> sessionTable_; // 세션 테이블(ID→포인터)
    std::mutex sessionMtx_;                  // 세션 테이블 보호용 뮤텍스
    std::atomic<uint64_t> sessionIdGen_;     // 세션 ID 발급기
    int acceptOutstanding_;                  // 동시에 등록할 AcceptEx 개수
    int workerCount_;                        // 워커 스레드 수

    // 통계 변수(접속 수, 초당 accept/recv/send)
    std::atomic<int> statCurConn_;
    std::atomic<int> statAccept_;
    std::atomic<int> statRecv_;
    std::atomic<int> statSend_;
};
