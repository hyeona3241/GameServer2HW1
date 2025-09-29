// IOCP_EchoServer.cpp
#include "IOCP_EchoServer.h"

// -------------------- BufferPool 구현 --------------------

// 고정 크기 버퍼 풀 생성자: 미리 지정된 개수만큼 버퍼를 할당해둠
BufferPool::BufferPool(size_t blockSize, size_t initialCount) : blockSize_(blockSize) {
    for (size_t i = 0; i < initialCount; ++i)
        pool_.push_back(new char[blockSize_]);
}

// 버퍼 풀 소멸자: 남아있는 버퍼 모두 해제
BufferPool::~BufferPool() {
    for (auto* p : pool_) delete[] p;
}

// 버퍼 임대: 풀에 남은 버퍼가 있으면 반환, 없으면 새로 할당
char* BufferPool::Allocate() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (pool_.empty()) return new char[blockSize_];
    char* buf = pool_.back(); pool_.pop_back();
    return buf;
}

// 버퍼 반납: 사용이 끝난 버퍼를 풀에 다시 넣음
void BufferPool::Release(char* buf) {
    std::lock_guard<std::mutex> lock(mtx_);
    pool_.push_back(buf);
}

// -------------------- IOCP_EchoServer 구현 --------------------

// 생성자: 포트, 워커 스레드 수, AcceptEx 개수 등 초기화
IOCP_EchoServer::IOCP_EchoServer(unsigned short port, int workerCount, int acceptCount)
    : listenSocket_(INVALID_SOCKET), iocpHandle_(NULL), running_(false), port_(port),
    fnAcceptEx_(nullptr), fnGetAcceptExSockaddrs_(nullptr),
    bufferPool_(BUFFER_SIZE, 128), sessionIdGen_(1),
    acceptOutstanding_(acceptCount ? acceptCount : DEFAULT_ACCEPT),
    workerCount_(workerCount ? workerCount : (int)std::thread::hardware_concurrency()),
    statCurConn_(0), statAccept_(0), statRecv_(0), statSend_(0)
{
}

// 소멸자: Stop() 호출로 리소스 정리
IOCP_EchoServer::~IOCP_EchoServer() {
    Stop();
}

// 서버 시작: 소켓 생성, 바인드, 리슨, IOCP/확장함수/스레드/AcceptEx 준비
bool IOCP_EchoServer::Start() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;

    // WSA_FLAG_OVERLAPPED로 리슨 소켓 생성
    listenSocket_ = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listenSocket_ == INVALID_SOCKET) return false;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    if (bind(listenSocket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return false;
    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) return false;

    // IOCP 객체 생성 및 리슨 소켓 연결
    iocpHandle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (!iocpHandle_) return false;
    CreateIoCompletionPort((HANDLE)listenSocket_, iocpHandle_, 0, 0);

    // 확장 함수(AcceptEx 등) 로딩
    LoadExtensionFunctions();

    running_ = true;

    // AcceptEx를 미리 여러 개 등록(동시 접속 대비)
    for (int i = 0; i < acceptOutstanding_; ++i) PostAccept();

    // 워커 스레드 시작
    for (int i = 0; i < workerCount_; ++i)
        workerThreads_.emplace_back(&IOCP_EchoServer::WorkerThread, this);

    // 통계 출력 스레드 시작
    statThread_ = std::thread(&IOCP_EchoServer::StatThread, this);

    std::cout << "[INFO] 서버 시작, 포트: " << port_ << ", 워커: " << workerCount_ << ", AcceptEx: " << acceptOutstanding_ << std::endl;
    return true;
}

// 서버 정지: 스레드 종료, 소켓/핸들/세션/버퍼 정리
void IOCP_EchoServer::Stop() {
    if (!running_) return;
    running_ = false;

    closesocket(listenSocket_);
    // 워커 스레드 깨우기 위해 더미 완료 패킷 전송
    for (int i = 0; i < workerCount_; ++i)
        PostQueuedCompletionStatus(iocpHandle_, 0, 0, nullptr);

    for (auto& t : workerThreads_) {
        if (t.joinable()) t.join();
    }
    if (statThread_.joinable()) statThread_.join();

    // 남은 세션 모두 종료
    {
        std::lock_guard<std::mutex> lock(sessionMtx_);
        for (auto& kv : sessionTable_) {
            CloseSession(kv.second);
        }
        sessionTable_.clear();
    }

    CloseHandle(iocpHandle_);
    WSACleanup();
}

// AcceptEx, GetAcceptExSockaddrs 확장 함수 포인터 로딩
void IOCP_EchoServer::LoadExtensionFunctions() {
    GUID guidAcceptEx = WSAID_ACCEPTEX;
    DWORD bytes = 0;
    WSAIoctl(listenSocket_, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidAcceptEx, sizeof(guidAcceptEx),
        &fnAcceptEx_, sizeof(fnAcceptEx_),
        &bytes, NULL, NULL);

    GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
    WSAIoctl(listenSocket_, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidGetAcceptExSockaddrs, sizeof(guidGetAcceptExSockaddrs),
        &fnGetAcceptExSockaddrs_, sizeof(fnGetAcceptExSockaddrs_),
        &bytes, NULL, NULL);

    if (!fnAcceptEx_ || !fnGetAcceptExSockaddrs_) {
        std::cerr << "[ERROR] 확장 함수 로딩 실패\n";
        exit(-1);
    }
    std::cout << "[INFO] 확장 함수 로딩 성공\n";
}

// AcceptEx를 미리 등록(선게시)하여 클라이언트 접속을 기다림
void IOCP_EchoServer::PostAccept() {
    auto* ov = new OverlappedEx();
    ov->op = OverlappedEx::OP_ACCEPT;
    ov->acceptSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    DWORD bytes = 0;
    BOOL ret = fnAcceptEx_(
        listenSocket_, ov->acceptSock,
        ov->buffer, 0,
        sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
        &bytes, ov
    );
    // ERROR_IO_PENDING이 아니면 에러(즉시 실패)
    if (!ret && WSAGetLastError() != ERROR_IO_PENDING) {
        closesocket(ov->acceptSock);
        delete ov;
        std::cerr << "[ERROR] AcceptEx 선게시 실패\n";
        // 실패 시 재시도
        PostAccept();
    }
}

// AcceptEx 완료 시 호출: 새 세션 생성, IOCP 등록, 첫 수신 게시, AcceptEx 재등록
void IOCP_EchoServer::AcceptCompletion(OverlappedEx* ov, DWORD /*bytes*/) {
    // SO_UPDATE_ACCEPT_CONTEXT: 소켓을 accept 소켓으로 전환
    setsockopt(ov->acceptSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenSocket_, sizeof(listenSocket_));

    // 새 세션 생성 및 초기화
    auto* session = new Session();
    session->sock = ov->acceptSock;
    session->rxBuf = bufferPool_.Allocate();
    session->rxUsed = 0;
    session->id = sessionIdGen_++;
    session->sending = false;
    session->closing = false;

    // 세션 테이블에 등록(동시성 보호)
    {
        std::lock_guard<std::mutex> lock(sessionMtx_);
        sessionTable_[session->id] = session;
    }
    statCurConn_++;
    statAccept_++;

    // 세션 소켓을 IOCP에 등록(키: 세션 포인터)
    CreateIoCompletionPort((HANDLE)session->sock, iocpHandle_, (ULONG_PTR)session, 0);

    // 첫 수신 요청 등록
    PostRecv(session);

    // 다음 AcceptEx 즉시 재등록(항상 N개 유지)
    PostAccept();

    delete ov;
}

// 클라이언트로부터 데이터 수신 요청(WSARecv) 등록
void IOCP_EchoServer::PostRecv(Session* session) {
    if (session->closing) return;
    ZeroMemory(&session->ovRecv, sizeof(OverlappedEx));
    session->ovRecv.op = OverlappedEx::OP_RECV;
    session->ovRecv.session = session;
    session->ovRecv.wsaBuf.buf = session->rxBuf + session->rxUsed;
    session->ovRecv.wsaBuf.len = BUFFER_SIZE - (ULONG)session->rxUsed;

    DWORD flags = 0, recvBytes = 0;
    int ret = WSARecv(session->sock, &session->ovRecv.wsaBuf, 1, &recvBytes, &flags, &session->ovRecv, NULL);
    // ERROR_IO_PENDING이 아니면 에러(즉시 종료)
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        CloseSession(session);
    }
}

// 수신 완료 시 호출: 데이터 처리(채팅 브로드캐스트), 다음 수신 요청 등록
void IOCP_EchoServer::RecvCompletion(Session* session, DWORD bytes) {
    if (bytes == 0) {
        // bytes==0: 정상 종료(상대방이 연결 종료)
        CloseSession(session);
        return;
    }
    session->rxUsed += bytes;
    statRecv_++;

    // --- 채팅: 모든 세션에 메시지 브로드캐스트 ---
    std::vector<Session*> sessions;
    {
        std::lock_guard<std::mutex> lock(sessionMtx_);
        for (auto& kv : sessionTable_) {
            sessions.push_back(kv.second);
        }
    }
    for (Session* s : sessions) {
        // 자기 자신에게도 메시지 전송(원하면 if (s != session)로 제외 가능)
        if (s != session)
            EnqueueSend(s, session->rxBuf, session->rxUsed);
    }
    session->rxUsed = 0;

    // 다음 수신 요청 등록(연속 수신)
    PostRecv(session);
}
// 송신 큐에 데이터 추가, 송신 중이 아니면 송신 시작
void IOCP_EchoServer::EnqueueSend(Session* session, const char* data, size_t len) {
    if (session->closing) return;
    {
        std::lock_guard<std::mutex> lock(sessionMtx_);
        session->sendQueue.push(std::vector<char>(data, data + len));
    }
    // sending이 false였으면 true로 바꾸고 송신 시작
    if (!session->sending.exchange(true)) {
        PostSend(session);
    }
}

// 송신 요청(WSASend) 등록, 큐에 여러 패킷이 있으면 병합해서 보냄
void IOCP_EchoServer::PostSend(Session* session) {
    if (session->closing) return;
    std::vector<WSABUF> bufs;
    std::vector<std::vector<char>> toSend;
    {
        std::lock_guard<std::mutex> lock(sessionMtx_);
        int cnt = 0;
        // 최대 16개까지 패킷 병합
        while (!session->sendQueue.empty() && cnt < 16) {
            toSend.push_back(std::move(session->sendQueue.front()));
            session->sendQueue.pop();
            cnt++;
        }
    }
    if (toSend.empty()) {
        session->sending = false;
        return;
    }
    // bufs는 toSend의 데이터를 가리킴
    for (auto& v : toSend) {
        WSABUF b;
        b.buf = v.data();
        b.len = (ULONG)v.size();
        bufs.push_back(b);
    }
    session->ovSend = OverlappedEx();
    session->ovSend.op = OverlappedEx::OP_SEND;
    session->ovSend.session = session;

    DWORD sentBytes = 0;
    int ret = WSASend(session->sock, bufs.data(), (DWORD)bufs.size(), &sentBytes, 0, &session->ovSend, NULL);
    // ERROR_IO_PENDING이 아니면 에러(즉시 종료)
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        CloseSession(session);
    }
    // toSend의 데이터는 WSASend 완료 후 해제됨(스택)
}

// 송신 완료 시 호출: 큐에 남은 데이터가 있으면 재송신, 없으면 sending=false
void IOCP_EchoServer::SendCompletion(Session* session, DWORD /*bytes*/) {
    statSend_++;
    // 큐에 남은 데이터가 있으면 다시 송신
    PostSend(session);
}

// 세션 종료 및 리소스 반환(이중 종료 방지)
void IOCP_EchoServer::CloseSession(Session* session) {
    if (!session || session->closing.exchange(true)) return;
    closesocket(session->sock);
    bufferPool_.Release(session->rxBuf);
    {
        std::lock_guard<std::mutex> lock(sessionMtx_);
        sessionTable_.erase(session->id);
    }
    statCurConn_--;
    delete session;
}

// IOCP 워커 스레드: 모든 I/O 완료 이벤트를 처리
void IOCP_EchoServer::WorkerThread() {
    while (running_) {
        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0;
        LPOVERLAPPED overlapped = nullptr;

        // IOCP에서 완료 이벤트 대기
        BOOL result = GetQueuedCompletionStatus(iocpHandle_, &bytesTransferred, &completionKey, &overlapped, INFINITE);
        if (!running_) break;
        if (!result && !overlapped) continue;
        if (!overlapped) continue;

        OverlappedEx* ov = reinterpret_cast<OverlappedEx*>(overlapped);

        // 작업 종류에 따라 분기 처리
        if (ov->op == OverlappedEx::OP_ACCEPT) {
            AcceptCompletion(ov, bytesTransferred);
        }
        else if (ov->op == OverlappedEx::OP_RECV) {
            RecvCompletion(static_cast<Session*>(ov->session), bytesTransferred);
        }
        else if (ov->op == OverlappedEx::OP_SEND) {
            SendCompletion(static_cast<Session*>(ov->session), bytesTransferred);
        }
    }
}

// 통계 출력 스레드: 1초마다 접속 수, 초당 accept/recv/send 횟수 출력
void IOCP_EchoServer::StatThread() {
    using namespace std::chrono;
    auto last = steady_clock::now();
    int lastAccept = 0, lastRecv = 0, lastSend = 0;
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto now = steady_clock::now();
        int curConn = statCurConn_.load();
        int acc = statAccept_.exchange(0);
        int rx = statRecv_.exchange(0);
        int tx = statSend_.exchange(0);
        std::cout << "[STAT] Conn: " << curConn
            << " Accept/s: " << acc
            << " Recv/s: " << rx
            << " Send/s: " << tx << std::endl;
        last = now;
    }
}
