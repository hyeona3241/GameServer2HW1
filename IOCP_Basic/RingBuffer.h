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

    // ����
    size_t capacity() const noexcept { return buf_.size(); }
    size_t readable() const noexcept { return size_; }
    size_t writable() const noexcept { return buf_.size() - size_; }
    bool   empty()    const noexcept { return size_ == 0; }
    void   clear()          noexcept { head_ = tail_ = size_ = 0; }

    // ����: src���� �ִ� n����Ʈ�� �����Ͽ� ����. ���� ����� ����Ʈ �� ��ȯ.
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

    // �б�: �ִ� len ����Ʈ�� dst�� ����/�Һ�. ���� ���� ����Ʈ �� ��ȯ.
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

    // �̸�����: len ����Ʈ�� ����� ������ dst�� ����(�Һ� X)�ϰ� true, �����ϸ� false.
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

    // ���: len ����Ʈ�� �Һ�(���� X). ����� ������ true, �����ϸ� false.
    bool discard(size_t len) noexcept {
        if (len > size_) return false;

        head_ = (head_ + len) % buf_.size();
        size_ -= len;

        return true;
    }
};