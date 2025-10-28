#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cstring>

class RingBuffer
{
private:
    std::vector<std::uint8_t> buf_;
    size_t head_; 
    size_t tail_; 
    size_t size_;

public:
    explicit RingBuffer(size_t capacity = 64 * 1024)
        : buf_(std::max<size_t>(capacity, 1024)), head_(0), tail_(0), size_(0) {
    }

    // 상태
    size_t capacity() const noexcept { return buf_.size(); }
    size_t readable() const noexcept { return size_; }
    size_t writable() const noexcept { return buf_.size() - size_; }
    bool   empty()    const noexcept { return size_ == 0; }
    void   clear()          noexcept { head_ = tail_ = size_ = 0; }

    // 쓰기: src에서 최대 n바이트를 복사하여 누적. 실제 기록한 바이트 수 반환.
    size_t append(const void* src, size_t n) noexcept {
        if (n == 0 || writable() == 0) return 0;
        const auto* p = static_cast<const std::uint8_t*>(src);

        size_t written = 0;
        while (written < n && size_ < buf_.size()) {
            size_t tillEnd = buf_.size() - tail_;
            size_t free = buf_.size() - size_;
            size_t span = std::min(tillEnd, free);
            size_t chunk = std::min(span, n - written);

            std::memcpy(&buf_[tail_], p + written, chunk);

            tail_ = (tail_ + chunk) % buf_.size();
            size_ += chunk;
            written += chunk;
        }
        return written;
    }

    // 읽기: 최대 len 바이트를 dst로 복사/소비. 실제 읽은 바이트 수 반환.
    size_t read(void* dst, size_t len) noexcept {
        if (len == 0 || size_ == 0) return 0;
        auto* out = static_cast<std::uint8_t*>(dst);

        size_t got = 0;
        while (got < len && size_ > 0) {
            size_t tillEnd = buf_.size() - head_;
            size_t span = std::min(tillEnd, size_);
            size_t chunk = std::min(span, len - got);

            std::memcpy(out + got, &buf_[head_], chunk);

            head_ = (head_ + chunk) % buf_.size();
            size_ -= chunk;
            got += chunk;
        }
        return got;
    }

    // 미리보기: len 바이트가 충분히 있으면 dst로 복사(소비 X)하고 true, 부족하면 false.
    bool peek(void* dst, size_t len) const noexcept {
        if (len > size_) return false;
        if (len == 0)    return true;
        auto* out = static_cast<std::uint8_t*>(dst);

        size_t h = head_;
        size_t remain = len;
        size_t firstSpan = std::min(buf_.size() - h, size_);
        size_t first = std::min(firstSpan, remain);

        std::memcpy(out, &buf_[h], first);

        h = (h + first) % buf_.size();
        remain -= first;

        if (remain > 0) {
            std::memcpy(out + first, &buf_[h], remain);
        }
        return true;
    }

    // 폐기: len 바이트를 소비만(복사 X). 충분히 있으면 true, 부족하면 false.
    bool discard(size_t len) noexcept {
        if (len > size_) return false;

        head_ = (head_ + len) % buf_.size();
        size_ -= len;

        return true;
    }
};