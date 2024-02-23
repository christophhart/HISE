// Remove all warnings and make the loris library compile

// C++17 removes auto_ptr and some other helper classes so we'll make it compile with these definitions...
#if __cplusplus >= 201703L
#if _WIN32
namespace std
{
	template <typename U1, typename U2, typename U3> struct binary_function
	{
		using argument_type = U2;
	};
	template <typename U1, typename U2> struct unary_function
	{
		using argument_type = U2;
	};
}

#define bind1st bind
#define bind2nd bind

#endif
#define auto_ptr unique_ptr
#endif

#include "../JUCE/modules/juce_core/juce_core.h"
