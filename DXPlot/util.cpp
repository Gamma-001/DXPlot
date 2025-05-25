#include <util.hpp>

#include <cstdarg>
#include <sstream>

const char* Cass::ComException::what() const {
	std::ostringstream ss;
	ss << "Faliure, HRESULT : " << std::hex << result;
	std::string str = ss.str();
	return str.c_str();
}

#if defined(_DEBUG) || defined(DEBUG)
void Cass::DebugLog(const char* fmt, ...) {
	std::ostringstream ss;
	va_list args;
	va_start(args, fmt);

	while (*fmt != '\0') {
		switch (*fmt) {
		case '%':
			switch (*(++fmt)) {
			case 'd':
				ss << va_arg(args, int64_t); break;

			case 'u':
				ss << va_arg(args, uint64_t); break;

			case 'f':
				ss << va_arg(args, double); break;

			case 'c':
				ss << static_cast <char> (va_arg(args, int)); break;

			case 's':
				ss << va_arg(args, char*); break;

			default: 
				ss << '%'; break;
			}
			break;
		
		default:
			ss << *fmt; break;
		}

		fmt++;
	}

	va_end(args);
	OutputDebugStringA(ss.str().c_str());
}
#else 
void Cass::DebugLog(const char* fmt, ...) { }
#endif

RECT Cass::GetAbsoluteClientRect(HWND hWnd) {
	RECT rc = { 0, 0, 0, 0 };
	POINT temp;

	GetClientRect(hWnd, &rc);

	temp = { rc.left, rc.top };
	ClientToScreen(hWnd, &temp);
	rc.left = temp.x;
	rc.top = temp.y;

	temp = { rc.right, rc.bottom };
	ClientToScreen(hWnd, &temp);
	rc.right = temp.x;
	rc.bottom = temp.y;

	return rc;
}