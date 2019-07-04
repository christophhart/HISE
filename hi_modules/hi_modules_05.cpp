
#include "JuceHeader.h"

#if HI_ENABLE_CUSTOM_NODE_LOCATION

#include "nodes/CustomNodeInclude.cpp"

namespace scriptnode
{
using namespace hise;
using namespace juce;

namespace custom
{
DEFINE_FACTORY_FOR_NAMESPACE;
}

#if USE_BACKEND

// For HISE we use an empty project factory or it will create a linker error.
namespace project
{
DEFINE_FACTORY_FOR_NAMESPACE
}
#endif

}



#endif

