#pragma once

// ����
#include <cstdint>
#include <string>

// ������ ���� ����
#if defined(_WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
#endif

#if defined(CORE_SERVER)

	// ���� ����
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
	#include "PKUserCountReq.h"
	#include "PKUserCountAck.h"
	#include "PKLoginReq.h"
	#include "PKLoginAck.h"
	#include "PKChatBroadcast.h"

#elif defined(CORE_CLIENT)

	// Ŭ�� ����
	#if defined(_WIN32)
		#include <winsock2.h>
		#include <ws2tcpip.h>
		#pragma comment(lib, "ws2_32.lib")
	#endif
	#include <iostream>
	#include <mutex>
	#include <queue>
	#include <chrono>

	#include "PKChatMessage.h"
	#include "PKExit.h"
	#include "PKUserCount.h"
	#include "PKUserCountReq.h"
	#include "PKUserCountAck.h"
	#include "PKLoginReq.h"
	#include "PKLoginAck.h"
	#include "PKChatBroadcast.h"

#else
	// �ƹ� Ÿ�굵 ���� �� ���� �� ���
	#pragma message("Define CORE_SERVER or CORE_CLIENT for proper includes.")
#endif

namespace NetConfig {

	// ���� ��Ʈ��ũ ����
	inline constexpr const char* SERVER_IP = "127.0.0.1";
	inline constexpr unsigned short SERVER_PORT = 9000;

	// ���� ����
	// �� ���� ����� ���� ũ��(4KB)
	inline constexpr int BUFFER_SIZE = 4096;
	// ���ÿ� �̸� �޾Ƶ� AcceptEx ����(�⺻ 16, �ھ� ���� ���� ���� ����)
	inline constexpr int DEFAULT_ACCEPT = 16; 

	// �޽��� ����
	inline constexpr const char* QUIT_COMMAND = "q"; // Ŭ�� ���� ���
}