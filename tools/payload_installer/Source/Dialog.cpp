

#include "Dialog.h"

namespace hise {
namespace multipage {
using namespace juce;
Dialog* PayloadInstaller::createDialog(State& state)
{
	auto f = File::getSpecialLocation(File::SpecialLocationType::currentApplicationFile);

	auto payloadFile = f.withFileExtension(".dat");

	if(payloadFile.existsAsFile())
	{
		FileInputStream fis(payloadFile);
        try
        {
            return MonolithData(&fis).create(state, false);
        }
        catch(String& s)
        {
            NativeMessageBox::showMessageBox(MessageBoxIconType::WarningIcon, "Error loading payload", s);
            
            JUCEApplication::getInstance()->systemRequestedQuit();
            return nullptr;
        }
        
	}
	else
	{
		NativeMessageBox::showMessageBox(MessageBoxIconType::WarningIcon, "Can't find payload", "Please locate the installer payload on your system");

		FileChooser fc("Locate installer payload", f.getParentDirectory(), f.withFileExtension(".dat").getFileName());

		if(fc.browseForFileToOpen())
		{
			auto f = fc.getResult();

			if(f.getFileName() == payloadFile.getFileName())
			{
				FileInputStream fis(f);
                
                try
                {
                    return MonolithData(&fis).create(state, false);
                }
                catch(String& s)
                {
                    NativeMessageBox::showMessageBox(MessageBoxIconType::WarningIcon, "Error loading payload", s);
                    
                    JUCEApplication::getInstance()->systemRequestedQuit();
                    return nullptr;
                }
			}
		}
	}

	NativeMessageBox::showMessageBox(MessageBoxIconType::WarningIcon, "Can't find payload", "The installer payload could not be located. The application will terminate.");
	
	JUCEApplication::getInstance()->systemRequestedQuit();

	return nullptr;
}
} // namespace multipage
} // namespace hise
