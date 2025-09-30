#pragma once

// 공통
#include <cstdint>
#include <string>

// 윈도우 전용 가드
#if defined(_WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
#endif

#if defined(CORE_SERVER)

	// 서버 전용
	#if defined(_WIN32)
		#include <winsock2.h> 
		#include <mswsock.h>
		#include <windows.h>
		#pragma comment(lib, "ws2_32.lib")
	#endif
	#include <vector>
	#include <queue>
	#include <mutex>
	#include <atomic>
	#include <unordered_map>
	#include <thread>
	#include <chrono>
	#include <iostream>

	#include "PKChatMessage.h"
	#include "PKExit.h"
	#include "PKUserCount.h"

#elif defined(CORE_CLIENT)

	// 클라 전용
	#if defined(_WIN32)
		#include <winsock2.h>
		#include <ws2tcpip.h>
		#pragma comment(lib, "ws2_32.lib")
	#endif
	#include <iostream>
	#include <mutex>
	#include <queue>

	#include "PKChatMessage.h"
	#include "PKExit.h"
	#include "PKUserCount.h"

#else
	// 아무 타깃도 지정 안 됐을 때 경고
	#pragma message("Define CORE_SERVER or CORE_CLIENT for proper includes.")
#endif

namespace NetConfig {

	// 공통 네트워크 설정
	inline constexpr const char* SERVER_IP = "127.0.0.1";
	inline constexpr unsigned short SERVER_PORT = 9000;

	// 버퍼 관련
	// 한 번에 사용할 버퍼 크기(4KB)
	inline constexpr int BUFFER_SIZE = 4096;
	// 동시에 미리 받아둘 AcceptEx 개수(기본 16, 코어 수에 따라 조정 가능)
	inline constexpr int DEFAULT_ACCEPT = 16; 

	// 메시지 관련
	inline constexpr const char* QUIT_COMMAND = "q"; // 클라 종료 명령
}