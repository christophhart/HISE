#ifndef MAINCONTROLLERSHELL_H_INCLUDED
#define MAINCONTROLLERSHELL_H_INCLUDED
// Derive from this class if you want to log in a class which MainController
// depends on.

namespace hise {
// forward declaration of MainController
class MainController;

// derive from these classes if you need access to various methods of
// MainController before it is actually defined and MainController
// depends on your class.

class InternalLogger {
public:
    void logMessage(MainController* mc, const String& errorMessage);
};
} // namespace hise
#endif // MAINCONTROLLERSHELL_H_INCLUDED
