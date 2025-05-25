#pragma once

namespace Cass {
	class ControlKeys {
	public:
		bool l_ctrl, r_ctrl, l_shift, r_shift, tab, l_alt, r_alt;
		ControlKeys() :
			l_ctrl(false), r_ctrl(false),
			l_shift(false), r_shift(false),
			tab(false),
			l_alt(false), r_alt(false) {}
	};

	class Keyboard {
	public:
		static ControlKeys controls;
	};
}