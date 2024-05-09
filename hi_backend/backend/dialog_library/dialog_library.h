// Put in the header definitions of every dialog here...

#include "hi_backend/backend/BackendApplicationCommands.h"



namespace hise {
namespace multipage {
namespace library {
using namespace juce;
struct BroadcasterWizard: public HardcodedDialogWithState,
                          public hise::QuasiModalComponent
{
    BroadcasterWizard(BackendRootWindow* brw):
      bpe(brw)
    {
        closeFunction = BIND_MEMBER_FUNCTION_0(BroadcasterWizard::destroy);
        
        state.bindCallback("checkSelection", BIND_MEMBER_FUNCTION_1(BroadcasterWizard::checkSelection));
        
        setSize(832, 716);
    }
    
    var checkSelection(const var::NativeFunctionArgs& args);

    StringArray getAutocompleteItems(const Identifier& textEditorId);
    
    Dialog* createDialog(State& state) override;
    
    BackendRootWindow* bpe;

    bool addListenersOnly = false;
    String customId;
};
} // namespace library
} // namespace multipage
} // namespace hise

namespace hise {
namespace multipage {
namespace library {
using namespace juce;
struct SnippetBrowser: public HardcodedDialogWithState
{
	SnippetBrowser(BackendRootWindow* brw):
	  bpe(brw)
	{
		//closeFunction = BIND_MEMBER_FUNCTION_0(SnippetBrowser::destroy);
		setSize(450, 1200);
		state.bindCallback("loadSnippet", BIND_MEMBER_FUNCTION_1(SnippetBrowser::loadSnippet));
		state.bindCallback("exportSnippet", BIND_MEMBER_FUNCTION_1(SnippetBrowser::exportSnippet));
	}

    var exportSnippet(const var::NativeFunctionArgs& args)
	{
		auto content = BackendCommandTarget::Actions::exportFileAsSnippet(bpe, false);
        return var(content);
	}

	BackendRootWindow* bpe;

	var loadSnippet(const var::NativeFunctionArgs& args);

	Dialog* createDialog(State& state) override;
	
};


} // namespace library
} // namespace multipage
} // namespace hise