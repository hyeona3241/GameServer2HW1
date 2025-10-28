#include "Logger.h"
#include "PacketDef.h"
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace std;

Logger& Logger::Instance() {
    static Logger inst;
    return inst;
}

bool Logger::Init(const string& dir, const string& base) {
    namespace fs = std::filesystem;
    if (!fs::exists(dir)) fs::create_directories(dir);

    info_.open(fs::path(dir) / (base + ".log"), ios::out | ios::app);
    error_.open(fs::path(dir) / ("error.log"), ios::out | ios::app);

    return info_.is_open() && error_.is_open();
}

void Logger::Shutdown() {
    lock_guard<mutex> lock(mtx_);
    if (info_.is_open()) info_.close();
    if (error_.is_open()) error_.close();
}

string Logger::NowString() const {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto secs = time_point_cast<seconds>(now);
    const auto ms = duration_cast<milliseconds>(now - secs).count();

    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.'
        << std::setw(3) << std::setfill('0') << ms;
    return oss.str();
}

void Logger::WriteLineTo(std::ofstream& out, const char* levelTag, const std::string& msg)
{
    if (!out.is_open()) return;

    out << "[" << NowString() << "] " << "[" << levelTag << "] " << msg << '\n';
    out.flush();
}

void Logger::Info(const string& msg) {
    lock_guard<mutex> lock(mtx_);
    WriteLineTo(info_, "Info", msg);
}

void Logger::Warn(const std::string& msg)
{
    lock_guard<mutex> lock(mtx_);
    WriteLineTo(info_, "Warn", msg);
}

void Logger::Error(const string& msg) {
    lock_guard<mutex> lock(mtx_);
    WriteLineTo(error_, "Error", msg);
}

void Logger::Chat(const ChatMessageData& d, uint64_t sid) {
    std::ostringstream oss;
    oss << "Chat [session " << sid << "] [" << d.sender << "] " << d.message;
    Info(oss.str());
}