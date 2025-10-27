// IOCP_EchoServer.cpp
#include "IOCP_EchoServer.h"

// ������: ��Ʈ, ��Ŀ ������ ��, AcceptEx ���� �� �ʱ�ȭ
IOCP_EchoServer::IOCP_EchoServer(unsigned short port, int workerCount, int acceptCount, size_t maxSessions)
    : listenSocket_(INVALID_SOCKET), iocpHandle_(NULL), running_(false), port_(port),
    fnAcceptEx_(nullptr), fnGetAcceptExSockaddrs_(nullptr),
    bufferPool_(NetConfig::BUFFER_SIZE, 128), sessionIdGen_(1),
    acceptOutstanding_(acceptCount ? acceptCount : NetConfig::DEFAULT_ACCEPT),
    workerCount_(workerCount ? workerCount : (int)std::thread::hardware_concurrency()),
    statCurConn_(0), statAccept_(0), statRecv_(0), statSend_(0),
    sessionPool_(maxSessions)
{
}

// �Ҹ���: Stop() ȣ��� ���ҽ� ����
IOCP_EchoServer::~IOCP_EchoServer() {
    Stop();
}

// ���� ����: ���� ����, ���ε�, ����, IOCP/Ȯ���Լ�/������/AcceptEx �غ�
bool IOCP_EchoServer::Start() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;

    // WSA_FLAG_OVERLAPPED�� ���� ���� ����
    listenSocket_ = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listenSocket_ == INVALID_SOCKET) return false;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    if (bind(listenSocket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return false;
    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) return false;

    // IOCP ��ü ���� �� ���� ���� ����
    iocpHandle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (!iocpHandle_) return false;
    CreateIoCompletionPort((HANDLE)listenSocket_, iocpHandle_, 0, 0);

    // Ȯ�� �Լ�(AcceptEx ��) �ε�
    LoadExtensionFunctions();

    running_ = true;

    // AcceptEx�� �̸� ���� �� ���(���� ���� ���)
    for (int i = 0; i < acceptOutstanding_; ++i) PostAccept();

    // ��Ŀ ������ ����
    for (int i = 0; i < workerCount_; ++i)
        workerThreads_.emplace_back(&IOCP_EchoServer::WorkerThread, this);

    // ��� ��� ������ ����
    statThread_ = std::thread(&IOCP_EchoServer::StatThread, this);

    std::cout << "[INFO] ���� ����, ��Ʈ: " << port_ << ", ��Ŀ: " << workerCount_ << ", AcceptEx: " << acceptOutstanding_ << std::endl;

    Logger::Instance().Init("logs", "server");
    Logger::Instance().Info("Server started on port " + std::to_string(port_));
    return true;
}

// ���� ����: ������ ����, ����/�ڵ�/����/���� ����
void IOCP_EchoServer::Stop() {
    if (!running_) return;
    Logger::Instance().Info("Server stopping...");
    running_ = false;

    closesocket(listenSocket_);
    // ��Ŀ ������ ����� ���� ���� �Ϸ� ��Ŷ ����
    for (int i = 0; i < workerCount_; ++i)
        PostQueuedCompletionStatus(iocpHandle_, 0, 0, nullptr);

    for (auto& t : workerThreads_) {
        if (t.joinable()) t.join();
    }
    if (statThread_.joinable()) statThread_.join();

    // ���� ���� ��� ����
    {
        std::lock_guard<std::mutex> lock(sessionMtx_);
        for (auto& kv : sessionTable_) {
            CloseSession(kv.second);
        }
        sessionTable_.clear();
    }

    CloseHandle(iocpHandle_);
    WSACleanup();
    Logger::Instance().Shutdown();
}

// AcceptEx, GetAcceptExSockaddrs Ȯ�� �Լ� ������ �ε�
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
        std::cerr << "[ERROR] Ȯ�� �Լ� �ε� ����\n";
        exit(-1);
    }
    std::cout << "[INFO] Ȯ�� �Լ� �ε� ����\n";
}

// AcceptEx�� �̸� ���(���Խ�)�Ͽ� Ŭ���̾�Ʈ ������ ��ٸ�
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
    // ERROR_IO_PENDING�� �ƴϸ� ����(��� ����)
    if (!ret && WSAGetLastError() != ERROR_IO_PENDING) {
        closesocket(ov->acceptSock);
        delete ov;
        std::cerr << "[ERROR] AcceptEx ���Խ� ����\n";
        // ���� �� ��õ�
        PostAccept();
    }
}

// AcceptEx �Ϸ� �� ȣ��: �� ���� ����, IOCP ���, ù ���� �Խ�, AcceptEx ����
void IOCP_EchoServer::AcceptCompletion(OverlappedEx* ov, DWORD /*bytes*/) {
    // SO_UPDATE_ACCEPT_CONTEXT: ������ accept �������� ��ȯ
    setsockopt(ov->acceptSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listenSocket_, sizeof(listenSocket_));

    // �� ���� ���� �� �ʱ�ȭ
    Session* session = sessionPool_.Acquire();
    if (!session) {
        closesocket(ov->acceptSock);
        PostAccept();
        delete ov;
        return;
    
    }
    session->sock = ov->acceptSock;
    session->id = sessionIdGen_++;
    session->sending = false;
    session->closing = false;
    session->rx.clear();
    session->rxBuf = bufferPool_.Allocate();

    // ���� ���̺� ���(���ü� ��ȣ)
    {
        std::lock_guard<std::mutex> lock(sessionMtx_);
        sessionTable_[session->id] = session;
    }
    statCurConn_++;
    statAccept_++;

    // ���� ������ IOCP�� ���(Ű: ���� ������)
    CreateIoCompletionPort((HANDLE)session->sock, iocpHandle_, (ULONG_PTR)session, 0);

    // ù ���� ��û ���
    PostRecv(session);

    // ���� AcceptEx ��� ����(�׻� N�� ����)
    PostAccept();

    handler_.OnClientConnected(session);

    delete ov;
}

// Ŭ���̾�Ʈ�κ��� ������ ���� ��û(WSARecv) ���
void IOCP_EchoServer::PostRecv(Session* session) {
    if (session->closing) return;
    ZeroMemory(&session->ovRecv, sizeof(OverlappedEx));
    session->ovRecv.op = OverlappedEx::OP_RECV;
    session->ovRecv.session = session;
    session->ovRecv.wsaBuf.buf = session->rxBuf;
    session->ovRecv.wsaBuf.len = NetConfig::BUFFER_SIZE;

    DWORD flags = 0, recvBytes = 0;
    int ret = WSARecv(session->sock, &session->ovRecv.wsaBuf, 1, &recvBytes, &flags, &session->ovRecv, NULL);
    // ERROR_IO_PENDING�� �ƴϸ� ����(��� ����)
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        CloseSession(session);
    }
}

// ���� �Ϸ� �� ȣ��: ������ ó��(ä�� ��ε�ĳ��Ʈ), ���� ���� ��û ���
void IOCP_EchoServer::RecvCompletion(Session* session, DWORD bytes) {
    if (bytes == 0) {
        // bytes==0: ���� ����(������ ���� ����)
        CloseSession(session);
        return;
    }
    /*session->rxUsed += bytes;
    statRecv_++;*/

    //// --- ä��: ��� ���ǿ� �޽��� ��ε�ĳ��Ʈ ---
    //std::vector<Session*> sessions;
    //{
    //    std::lock_guard<std::mutex> lock(sessionMtx_);
    //    for (auto& kv : sessionTable_) {
    //        sessions.push_back(kv.second);
    //    }
    //}
    //for (Session* s : sessions) {
    //    // �ڱ� �ڽſ��Ե� �޽��� ����(���ϸ� if (s != session)�� ���� ����)
    //    if (s != session)
    //        EnqueueSend(s, session->rxBuf, session->rxUsed);
    //}
    //session->rxUsed = 0;

    /*handler_.OnBytes(session, session->rxBuf, session->rxUsed);
    session->rxUsed = 0;*/

    statRecv_++;

    // ���� ����Ʈ�� ���� �����ۿ� ����
    const size_t appended = session->rx.append(session->rxBuf, (size_t)bytes);
    if (appended < bytes) {
        /*Logger::Instance().Warn("RingBuffer overflow: id=" + std::to_string(session->id));*/
        CloseSession(session);
        return;
        
    }
    // �����̸ӷ� �ϼ� ��Ŷ�� ��� ���� ó��
    std::vector<char> pkt;
    for (;;) {
        auto r = framer_.Pop(session->rx, pkt); // Result::Ok / NeedMore / Malformed
        if (r == PacketFramer::Result::Ok) {
            handler_.OnPacket(session, pkt);
            continue;
            
        }
        if (r == PacketFramer::Result::NeedMore) break;
        // Malformed
        /*Logger::Instance().Warn("Malformed packet: id=" + std::to_string(session->id));*/
        CloseSession(session);
        return;
      
    }

    // ���� ���� ��û ���(���� ����)
    PostRecv(session);
}
// �۽� ť�� ������ �߰�, �۽� ���� �ƴϸ� �۽� ����
void IOCP_EchoServer::EnqueueSend(Session* session, const char* data, size_t len) {
    if (session->closing) return;
    {
        std::lock_guard<std::mutex> lock(sessionMtx_);
        session->sendQueue.push(std::vector<char>(data, data + len));
    }
    // sending�� false������ true�� �ٲٰ� �۽� ����
    if (!session->sending.exchange(true)) {
        PostSend(session);
    }
}

// �۽� ��û(WSASend) ���, ť�� ���� ��Ŷ�� ������ �����ؼ� ����
void IOCP_EchoServer::PostSend(Session* session) {
    if (session->closing) return;
    std::vector<WSABUF> bufs;
    std::vector<std::vector<char>> toSend;
    {
        std::lock_guard<std::mutex> lock(sessionMtx_);
        int cnt = 0;
        // �ִ� 16������ ��Ŷ ����
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
    // bufs�� toSend�� �����͸� ����Ŵ
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
    // ERROR_IO_PENDING�� �ƴϸ� ����(��� ����)
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        CloseSession(session);
    }
    // toSend�� �����ʹ� WSASend �Ϸ� �� ������(����)
}

// �۽� �Ϸ� �� ȣ��: ť�� ���� �����Ͱ� ������ ��۽�, ������ sending=false
void IOCP_EchoServer::SendCompletion(Session* session, DWORD /*bytes*/) {
    statSend_++;
    // ť�� ���� �����Ͱ� ������ �ٽ� �۽�
    PostSend(session);
}

// ���� ���� �� ���ҽ� ��ȯ(���� ���� ����)
void IOCP_EchoServer::CloseSession(Session* session) {
    if (!session || session->closing.exchange(true)) return;
    closesocket(session->sock);

    if (session->rxBuf) {
        bufferPool_.Release(session->rxBuf);
        session->rxBuf = nullptr;
        
    }

    {
        std::lock_guard<std::mutex> lock(sessionMtx_);
        sessionTable_.erase(session->id);
    }

    statCurConn_--;
    sessionPool_.Release(session);
}

int IOCP_EchoServer::GetCurrentConnectionCount() const
{
    return statCurConn_.load();
}

void IOCP_EchoServer::Broadcast(const char* data, size_t len, Session* exclude)
{
    std::vector<Session*> snapshot;
    {
        std::lock_guard<std::mutex> lock(sessionMtx_);
        snapshot.reserve(sessionTable_.size());
        for (auto& kv : sessionTable_) snapshot.push_back(kv.second);
    }
    for (Session* s : snapshot) {
        if (s == exclude) continue;
        EnqueueSend(s, data, len); 
    }
}

// IOCP ��Ŀ ������: ��� I/O �Ϸ� �̺�Ʈ�� ó��
void IOCP_EchoServer::WorkerThread() {
    while (running_) {
        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0;
        LPOVERLAPPED overlapped = nullptr;

        // IOCP���� �Ϸ� �̺�Ʈ ���
        BOOL result = GetQueuedCompletionStatus(iocpHandle_, &bytesTransferred, &completionKey, &overlapped, INFINITE);
        if (!running_) break;
        if (!result && !overlapped) continue;
        if (!overlapped) continue;

        OverlappedEx* ov = reinterpret_cast<OverlappedEx*>(overlapped);

        // �۾� ������ ���� �б� ó��
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

// ��� ��� ������: 1�ʸ��� ���� ��, �ʴ� accept/recv/send Ƚ�� ���
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
