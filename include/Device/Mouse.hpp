#pragma once

#include <windows.h>

namespace DX {
	class Mouse {
	public:
		static long posX, posY;
		static int wheelDelta;
		static bool pressedLB, pressedMB, pressedRB;
	};
}