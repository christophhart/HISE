
#include "JuceHeader.h"

#if HI_ENABLE_CUSTOM_NODE_LOCATION

#include "nodes/CoreNodes.h"
#include "nodes/MathNodes.h"
#include "nodes/DynamicsNode.h"
#include "nodes/EventNodes.h"
#include "nodes/FilterNode.h"
#include "nodes/RoutingNodes.h"
#include "nodes/DelayNode.h"
#include "nodes/MetaNodes.h"

#include "nodes/CustomNodeInclude.cpp"

namespace scriptnode
{
using namespace hise;
using namespace juce;

namespace custom
{
DEFINE_FACTORY_FOR_NAMESPACE;
}
}

#endif

