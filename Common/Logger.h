#pragma once

#include <string>
#include <fstream>
#include <mutex>

struct ChatMessageData;

class Logger
{
private:
    std::ofstream info_;
    std::ofstream error_;
    std::mutex mtx_;

public:
    static Logger& Instance();

    bool Init(const std::string& dir = "logs", const std::string& base = "server");
    void Shutdown();

    void Info(const std::string& msg);
    void Warn(const std::string& msg);
    void Error(const std::string& msg);

    // ä�� �α׵� Info�� �����ϰ� �Ϲ� �α� ���Ͽ� ���
    void Chat(const ChatMessageData& d, uint64_t sid);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string NowString() const;

    void WriteLineTo(std::ofstream& out, const char* levelTag, const std::string& msg);
};

