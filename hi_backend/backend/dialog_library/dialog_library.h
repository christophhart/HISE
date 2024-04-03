// Put in the header definitions of every dialog here...

namespace hise {
namespace multipage {
namespace library {
using namespace juce;
struct SnippetExporter: public HardcodedDialogWithState,
                        public hise::QuasiModalComponent
{
    var createMarkdownFile(State::Job& t, const var& state);
    SnippetExporter()
    {
        closeFunction = BIND_MEMBER_FUNCTION_0(SnippetExporter::destroy);
        setSize(800, 600);

    };
    
    Dialog* createDialog(State& state) override;
    
};
} // namespace library
} // namespace multipage
} // namespace hise



namespace hise {
namespace multipage {
namespace library {
using namespace juce;
struct BroadcasterWizard: public HardcodedDialogWithState,
                          public hise::QuasiModalComponent
{
    BroadcasterWizard()
    {
        closeFunction = BIND_MEMBER_FUNCTION_0(BroadcasterWizard::destroy);
        setSize(832, 716);
    }
    
    StringArray getAutocompleteItems(const Identifier& textEditorId);
    
    Dialog* createDialog(State& state) override;
    
};
} // namespace library
} // namespace multipage
} // namespace hise
