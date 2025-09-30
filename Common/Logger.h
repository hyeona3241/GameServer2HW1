#pragma once

#include <string>
#include <fstream>
#include <mutex>

struct ChatMessageData;

class Logger
{
public:
    static Logger& Instance();

    bool Init(const std::string& dir = "logs", const std::string& base = "server");
    void Shutdown();

    void Info(const std::string& msg);
    void Error(const std::string& msg);

    // 채팅 로그도 Info와 동일하게 일반 로그 파일에 기록
    void Chat(const ChatMessageData& d, uint64_t sid);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string NowString() const;

    std::ofstream info_;
    std::ofstream error_;
    std::mutex mtx_;
};

