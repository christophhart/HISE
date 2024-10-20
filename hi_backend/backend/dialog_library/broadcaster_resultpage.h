// Put in the header definitions of every dialog here...

namespace hise {
namespace multipage {
namespace library {
using namespace juce;

struct CustomResultPage: public Component,
                         public PlaceholderContentBase
{
    enum class SourceIndex
    {
        None,
        ComplexData,
        ComponentProperties,
        ComponentValue,
        ComponentVisibility,
        ContextMenu,
        EqEvents,
        ModuleParameters,
        MouseEvents,
        ProcessingSpecs,
        RadioGroup,
        RoutingMatrix,
        numSourceIndexTypes
    };

    enum class StringProcessor
    {
        None,
        Unquote,
        JoinToStringWithNewLines,
        FirstItemAsString,
        ParseInt,
        ParseArray,
        MultilineToVarArray,
        numStringProcessors
    };

    enum class TargetIndex
    {
        None,
        Callback,
        CallbackDelayed,
        ComponentProperty,
        ComponentRefresh,
        ComponentValue,
        ModuleParameter,
        numTargetIndexTypes
    };
    
    CustomResultPage(Dialog& r, const var& obj);;

    static var getArgs(SourceIndex source, const String& noneArgs);
    static String createFunctionBodyIfAnonymous(const String& functionName, SourceIndex sourceIndex, bool createValueFunction, const String& noneArgs);
	void appendLine(String& x, const var& state, const String& suffix, const Array<var>& args, Array<StringProcessor> sp={});
     String getTargetLine(TargetIndex target, const var& state);
     String getAttachLine(SourceIndex source, const var& state);

    String getVariableName() const;

    void postInit() override;
    void resized() override;
    Result checkGlobalState(var globalState) override;

    CodeDocument doc;
    mcl::TextDocument textDoc;
    mcl::TextEditor codeEditor;
    var gs;

};


} // namespace library
} // namespace multipage
} // namespace hise

