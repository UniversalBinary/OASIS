#include <boost/system/error_code.hpp>
#include <windows.h>
#include <winsock.h>

#ifndef _WIN32_ERROR
#define _WIN32_ERROR

// Wrap the Win32 error code so we have a distinct type.
struct win32_error_code
{
	DWORD error;
	explicit win32_error_code(DWORD e) noexcept
	{
		error = e;
	}

};

DWORD win32_from_HRESULT(HRESULT hr)
{
	if ((hr & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0))
	{
		return HRESULT_CODE(hr);
	}

	if (hr == S_OK)
	{
		return ERROR_SUCCESS;
	}

	// Not a Win32 HRESULT so return a generic error code.
	return ERROR_CAN_NOT_COMPLETE;
}

namespace oasis::system
{
	// The Win32 error code category.
	class win32_error_category : public boost::system::error_category
	{
	public:
		// Return a short descriptive name for the category.
		char const* name() const noexcept override final
		{
			return "Win32Error";
		}

		// Return what each error code means in text.
		std::string message(int c) const override final
		{
			char error[256];
			auto len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, static_cast<DWORD>(c), 0, error, sizeof error, NULL);
			if (len == 0) return "Unknown error";
			// trim trailing newline
			while (len && (error[len - 1] == '\r' || error[len - 1] == '\n'))
			{
				--len;
			}
			return std::string(error, len);
		}
	};
}

// Return a static instance of the custom category.
extern oasis::system::win32_error_category const& win32_error_category()
{
	static oasis::system::win32_error_category c;
	return c;
}

// Overload the global make_error_code() free function with our custom error. It will be found via ADL by the compiler if needed.
inline boost::system::error_code make_error_code(win32_error_code const& we)
{
	return boost::system::error_code(static_cast<int>(we.error), win32_error_category());
}

// Create an error_code from a Windows error code.
inline boost::system::error_code make_win32_error_code(DWORD e)
{
	return make_error_code(win32_error_code(e));
}

// Create an error_code from an HRESULT value.
inline boost::system::error_code make_win32_error_code(HRESULT hr)
{
	return make_error_code(win32_error_code(win32_from_HRESULT(hr)));
}

// Create an error_code from the last Windows error.
inline boost::system::error_code make_win32_error_code()
{
	return make_win32_error_code(GetLastError());
}

// Create an error_code from the last Winsock error.
inline boost::system::error_code make_winsock_error_code()
{
	return make_win32_error_code(static_cast<DWORD>(WSAGetLastError()));
}

namespace std
{
	// Tell the C++ 11 STL metaprogramming that win32_error_code is registered with the standard error code system.
	template <>
	struct is_error_code_enum<win32_error_code> : std::true_type {};
}

#endif
