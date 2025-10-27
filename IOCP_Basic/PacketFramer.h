#pragma once
#include <vector>
#include <cstdint>
#include "PacketDef.h"
#include "RingBuffer.h"

class PacketFramer
{
public:
    static constexpr size_t kHeaderSize = sizeof(PacketHeader);
    static constexpr size_t kMaxPacket = 32 * 1024; // 방어 상한(필요시 조정)

    enum class Result {
        Ok,         // 패킷 하나 성공적으로 pop 됨 (out에 헤더+바디 전부 채움)
        NeedMore,   // 아직 데이터가 더 필요함(헤더 or 바디 미도착)
        Malformed   // 비정상 헤더/사이즈(상위에서 세션 종료/로그 권장)
    };

    // 링버퍼에서 "완성된 패킷(헤더+바디)" 1개를 꺼내 out에 담는다.
    Result Pop(RingBuffer& rb, std::vector<char>& out) const noexcept {
        if (rb.readable() < kHeaderSize) return Result::NeedMore;

        // 헤더 미리보기(소비 X)
        PacketHeader h{};
        if (!rb.peek(&h, kHeaderSize)) return Result::NeedMore;

        // 헤더 유효성 검사
        if (h.size < kHeaderSize || h.size > kMaxPacket) {
            return Result::Malformed;
        }

        // 패킷 전체(헤더+바디)가 모두 도착했는지 확인
        if (rb.readable() < h.size) return Result::NeedMore;

        // 통째로 읽어 out에 담기(소비)
        out.resize(h.size);
        const size_t got = rb.read(out.data(), h.size);
        if (got != h.size) return Result::Malformed; // 로그 추가하기

        return Result::Ok;
    }
};

