#pragma once
#include <stack>
#include <mutex>
#include <vector>
#include "ServerTypes.h"

class SessionPool
{
private:
    size_t cap_;
    std::vector<Session*> pool_;
    std::stack<Session*> free_;
    std::mutex m_;

public:
    explicit SessionPool(size_t cap);
    ~SessionPool();

    Session* Acquire();
    void Release(Session* s);

};

// 세션 풀 초기화
inline SessionPool::SessionPool(size_t cap) : cap_(cap) {
    pool_.reserve(cap);
    for (size_t i = 0; i < cap; ++i) pool_.push_back(new Session());
    for (auto* s : pool_) free_.push(s);
}

// 세션 자원 정리
inline SessionPool::~SessionPool() {
    while (!free_.empty()) free_.pop();
    for (auto* s : pool_) delete s;
}

// 사용 가능한 세션을 하나 꺼내와서 반환
inline Session* SessionPool::Acquire() {
    std::lock_guard<std::mutex> g(m_);
    if (free_.empty()) return nullptr;

    Session* s = free_.top(); 
    free_.pop();

    s->rx.clear();
    s->closing.store(false);

    return s;
}

// 세션 반납
inline void SessionPool::Release(Session* s) {
    if (!s) return;

    std::lock_guard<std::mutex> g(m_);
    free_.push(s);
}