#pragma once
#include <cstdint>

namespace ecs
{
	using Entity = uint32_t;
	constexpr Entity nullentity = (Entity(-1));
}