

#include "JuceHeader.h"

#if INCLUDE_BIG_SCRIPTNODE_OBJECT_COMPILATION

#if USE_BACKEND
#include "nodes/MetaNodes.h"
#endif

#if HI_ENABLE_CUSTOM_NODE_LOCATION

#if USE_BACKEND
#include "nodes/CustomNodeInclude.cpp"
#endif

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

#endif