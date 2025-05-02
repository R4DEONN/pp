#pragma once

#include <cstdint>

struct PacketHeader {
	uint8_t type;
	uint32_t size;
};