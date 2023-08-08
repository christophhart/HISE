
namespace hise {
using namespace juce;
void InternalLogger::logMessage(MainController* mc, const String& errorMessage) {
    mc->getDebugLogger().logMessage(errorMessage);
}

} // namespace hise
