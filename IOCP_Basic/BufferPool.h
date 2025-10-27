#pragma once
#include <Utility.h>

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