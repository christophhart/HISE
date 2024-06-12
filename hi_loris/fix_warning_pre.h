// Remove all warnings and make the loris library compile

#if (defined (_WIN32) || defined (_WIN64))
#pragma warning( push )
#pragma warning( disable : 4505 )
#pragma warning( disable : 4267 )
#pragma warning( disable : 4189 )
#pragma warning( disable : 4018 )
#pragma warning( disable : 4334 )
#pragma warning( disable : 4189 )
#else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcomment"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wunused-variable"

#endif


#include "../JUCE/modules/juce_core/juce_core.h"
