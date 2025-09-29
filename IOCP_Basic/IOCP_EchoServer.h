// IOCP_EchoServer.h
#pragma once

#include <Utility.h>
#include "ServerTypes.h"

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
