/*
  ==============================================================================

    JavascriptApiClass.h
    Created: 3 Jul 2016 7:52:58pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef JAVASCRIPTAPICLASS_H_INCLUDED
#define JAVASCRIPTAPICLASS_H_INCLUDED

namespace hise { using namespace juce;


struct VariantComparator
{
	int compareElements(const var &a, const var &b) const
	{
		if (isNumericOrUndefined(a) && isNumericOrUndefined(b))
			return (a.isDouble() || b.isDouble()) ? returnCompareResult<double>(a, b) : returnCompareResult<int>(a, b);

		if ((a.isUndefined() || a.isVoid()) && (b.isUndefined() || b.isVoid()))
			return 0;

		if (a.isArray() || a.isObject())
			throw String("Can't compare arrays or objects");

		return 0;
	};

private:

	template <typename PrimitiveType> int returnCompareResult(const var &first, const var&second) const noexcept
	{
		PrimitiveType f = static_cast<PrimitiveType>(first);
		PrimitiveType s = static_cast<PrimitiveType>(second);
		return (f == s) ? 0 : (f > s ? 1 : -1);
	}

	bool isNumericOrUndefined(const var &v) const
	{
		return v.isDouble() || v.isInt() || v.isInt64() || v.isUndefined() || v.isBool();
	}
};

class IdentifierComparator
{
public:

	int compareElements(const Identifier &a, const Identifier &b) const
	{
		if (a.toString() > b.toString()) return 1;
		if (a.toString() < b.toString()) return -1;

		return 0;
	};
};


#define NUM_VAR_REGISTERS 32

#define DECLARE_ID(id) static const Identifier id(#id);

namespace VarTypeIdentifiers
{
    static const Identifier Undefined("undefined");
    static const Identifier Integer("int");
    static const Identifier Double("double");
    static const Identifier Number("number");
    static const Identifier String("string");
    DECLARE_ID(Colour);
    DECLARE_ID(Array);
    DECLARE_ID(Buffer);
    DECLARE_ID(ObjectWithLength);
    DECLARE_ID(JSON);
    DECLARE_ID(ScriptObject);
    DECLARE_ID(IndexOrArray);
    static const Identifier Object("object");
    DECLARE_ID(AudioData);
    DECLARE_ID(Function);
    DECLARE_ID(ComplexType);
    DECLARE_ID(NotUndefined);

    // TypedObjects
    DECLARE_ID(AudioFile);
    DECLARE_ID(AudioSampleProcessor);
    DECLARE_ID(BackgroundTask);
    DECLARE_ID(BeatportManager);
    DECLARE_ID(Broadcaster);
    
    DECLARE_ID(Builder);
    DECLARE_ID(ChildSynth);
    DECLARE_ID(ComplexGroupManager);
    DECLARE_ID(Connection);
    DECLARE_ID(ContainerChild);
    DECLARE_ID(DisplayBuffer);
    DECLARE_ID(DisplayBufferSource);
    DECLARE_ID(Download);
    DECLARE_ID(DspModule);
    DECLARE_ID(DspNetwork);
    DECLARE_ID(Effect);
    DECLARE_ID(ErrorHandler);
    DECLARE_ID(Expansion);
    DECLARE_ID(ExpansionHandler);
    DECLARE_ID(FFT);
    DECLARE_ID(File);
    DECLARE_ID(FixObjectArray);
    DECLARE_ID(FixObjectFactory);
    DECLARE_ID(FixObjectStack);
    DECLARE_ID(GlobalCable);
    DECLARE_ID(GlobalRoutingManager);
    DECLARE_ID(Graphics);
    DECLARE_ID(LorisManager);
    DECLARE_ID(MacroHandler);
    DECLARE_ID(MarkdownRenderer);
    DECLARE_ID(MessageHolder);
    DECLARE_ID(MidiAutomationHandler);
    DECLARE_ID(MidiList);
    DECLARE_ID(MidiPlayer);
    DECLARE_ID(MidiProcessor);
    DECLARE_ID(Modifiers);
    DECLARE_ID(Modulator);
    DECLARE_ID(NetworkTest);
    DECLARE_ID(NeuralNetwork);
    DECLARE_ID(Node);
    DECLARE_ID(Parameter);
    DECLARE_ID(Path);
    DECLARE_ID(Rectangle);
    DECLARE_ID(RoutingMatrix);
    DECLARE_ID(Sample);
    DECLARE_ID(ScriptAudioWaveform);
    DECLARE_ID(ScriptButton);
    DECLARE_ID(ScriptComboBox);
    DECLARE_ID(ScriptDynamicContainer);
    DECLARE_ID(ScriptFloatingTile);
    DECLARE_ID(ScriptImage);
    DECLARE_ID(ScriptLabel);
    DECLARE_ID(ScriptLookAndFeel);
    DECLARE_ID(ScriptModulationMatrix);
    DECLARE_ID(ScriptMultipageDialog);
    DECLARE_ID(ScriptPanel);
    DECLARE_ID(ScriptShader);
    DECLARE_ID(ScriptSlider);
    DECLARE_ID(ScriptSliderPack);
    DECLARE_ID(ScriptTable);
    DECLARE_ID(ScriptWebView);
    DECLARE_ID(ScriptedViewport);
    DECLARE_ID(SliderPackData);
    DECLARE_ID(SliderPackProcessor);
    DECLARE_ID(SlotFX);
    DECLARE_ID(Table);
    DECLARE_ID(TableProcessor);
    DECLARE_ID(ThreadSafeStorage);
    DECLARE_ID(Timer);
    DECLARE_ID(Unlocker);
    DECLARE_ID(UnorderedStack);
    DECLARE_ID(UserPresetHandler);
    DECLARE_ID(WavetableController);
}

#undef DECLARE_ID

/** A helper class that checks the type of var objects. */
struct VarTypeChecker
{
    // This is taken from the inline function
    static constexpr int NumMaxArgs = 5;
    
    enum VarTypes: uint8
    {
        Undefined = 0,
        Integer = 1,
        Double = 2,
        Number = 3,
        String = 4,
        Colour = String | Number,
        Array = 8,
        IndexOrArray = Array | Integer,
        Buffer = 16,
        AudioData = 8 | 16,
        ObjectWithLength = 4 | 8 | 16,
        JSON = 32,
        ScriptObject = 64,
        Object = 32 | 64,
        Function = 128,
        ComplexType = 4 | 8 | 16 | 32 | 64 | 128,
        NotUndefined = ComplexType | Number
    };

    enum class TypedObject: uint16
    {
        AudioFile = (uint16)VarTypes::NotUndefined + 1,
        AudioSampleProcessor,
        BackgroundTask,
        BeatportManager,
        Broadcaster,
        Buffer,
        Builder,
        ChildSynth,
        ComplexGroupManager,
        Connection,
        ContainerChild,
        DisplayBuffer,
        DisplayBufferSource,
        Download,
        DspModule,
        DspNetwork,
        Effect,
        ErrorHandler,
        Expansion,
        ExpansionHandler,
        FFT,
        File,
        FixObjectArray,
        FixObjectFactory,
        FixObjectStack,
        GlobalCable,
        GlobalRoutingManager,
        Graphics,
        LorisManager,
        MacroHandler,
        MarkdownRenderer,
        MessageHolder,
        MidiAutomationHandler,
        MidiList,
        MidiPlayer,
        MidiProcessor,
        Modifiers,
        Modulator,
        NetworkTest,
        NeuralNetwork,
        Node,
        Parameter,
        Path,
        Rectangle,
        RoutingMatrix,
        Sample,
        ScriptAudioWaveform,
        ScriptButton,
        ScriptComboBox,
        ScriptDynamicContainer,
        ScriptFloatingTile,
        ScriptImage,
        ScriptLabel,
        ScriptLookAndFeel,
        ScriptModulationMatrix,
        ScriptMultipageDialog,
        ScriptPanel,
        ScriptShader,
        ScriptSlider,
        ScriptSliderPack,
        ScriptTable,
        ScriptWebView,
        ScriptedViewport,
        SliderPackData,
        SliderPackProcessor,
        SlotFX,
        Table,
        TableProcessor,
        ThreadSafeStorage,
        Timer,
        Unlocker,
        UnorderedStack,
        UserPresetHandler,
        WavetableController
    };
    
    struct WrappedType
    {
        WrappedType() :
            value(0)
        {}

        explicit WrappedType(VarTypes t) :
            value((uint16)(uint8)t)
        {};

        explicit WrappedType(TypedObject o) :
            value((uint16)o)
        {};

        explicit operator bool() const { return value != 0; }

        bool isPrimitive() const
        {
            return value < (uint16)TypedObject::AudioFile;
        }

        TypedObject asComplexType() const
        {
            jassert(!isPrimitive());
                
            return (TypedObject)value;
        }

        VarTypes asPrimitiveType() const 
        {
            if (isPrimitive())
                return (VarTypes)(uint8)value;

            return VarTypes::ScriptObject;
        }

        bool matches(WrappedType other, bool allowUndefined) const
        {
            auto t = value;
            auto expectedType = other.value;

            bool ok;

            if (isPrimitive() && other.isPrimitive())
            {
                // both primitive, check bitmask
                ok = (t & expectedType) != 0;
            }
            else if (!isPrimitive() && !other.isPrimitive())
            {
                // both complex types, compare actual type index
                ok = t == expectedType;
            }
            else if (isPrimitive() != other.isPrimitive())
            {
                // one is primitive, the other is complex,
                // allow one of the types to be a complex type and the other to be a ScriptObject

                ok = (isPrimitive() && asPrimitiveType() == VarTypes::ScriptObject) ||
                      (other.isPrimitive() && other.asPrimitiveType() == VarTypes::ScriptObject);
            }

            if (!ok && allowUndefined)
                ok = t == VarTypes::Undefined;

            return ok;
        }

    private:

        uint16 value;
    };

    using ParameterTypes = std::array<WrappedType, NumMaxArgs>;
    
    static Identifier getTypeName(TypedObject to);

    static Identifier getTypeName(VarTypes type);

    static Identifier getTypeName(WrappedType wt)
    {
        if (wt.isPrimitive())
            return getTypeName(wt.asPrimitiveType());
        else
            return getTypeName(wt.asComplexType());
    }
    
    static WrappedType getTypeFromString(const juce::Identifier& name);
    
    static WrappedType getType(const var& value);
    
    static ParameterTypes createParameterTypes(VarTypes t1,
                                               VarTypes t2=VarTypes::Undefined,
                                               VarTypes t3=VarTypes::Undefined,
                                               VarTypes t4=VarTypes::Undefined,
                                               VarTypes t5=VarTypes::Undefined)
    {
        return {WrappedType(t1), WrappedType(t2), WrappedType(t3), WrappedType(t4), WrappedType(t5) };
    }
    
    ParameterTypes createParameterTypesFromNames(const StringArray& names)
    {
        ParameterTypes t;
        int index = 0;
        
        for(const auto& n: names)
        {
            if(index == NumMaxArgs)
            {
                jassertfalse;
                break;
            }
            
            t[index++] = getTypeFromString(n);
        }
        
        return t;
    }
    
    /** Creates a parameter type set for the given*/
    static ParameterTypes createParameterTypes(const juce::Array<var>& values)
    {
        ParameterTypes t;
        
        int index = 0;
        
        for(const auto& v: values)
        {
            if(index == NumMaxArgs)
            {
                // You can' use this with more than NumMaxArgs parameters
                jassertfalse;
                break;
            }
                
            t[index++] = getType(v);
        }
        
        return t;
    }
    
#if ENABLE_SCRIPTING_SAFE_CHECKS
    static Result checkType(const var& value, WrappedType expectedType, bool allowUndefined=false)
    {
        WrappedType t = getType(value);
        
        auto matches = t.matches(expectedType, allowUndefined);

#if 0
        auto matches = (t & expectedType) != 0;
        
        if(!matches && allowUndefined)
            matches = t == VarTypes::Undefined;
#endif
        
        if(!matches)
        {
            juce::String errorMessage;
            errorMessage << "Illegal type: " << getTypeName(getType(value));
            errorMessage << ", expected: " << getTypeName(expectedType);
            
            return Result::fail(errorMessage);
        }
        
        return Result::ok();
    }
#endif
};

/** A VarRegister is a container with a fixed amount of var slots that can be used for faster variable access in the Javascript engine. */
class VarRegister
{
public:

	// ================================================================================================================

	VarRegister();
	VarRegister(VarRegister &other);
	~VarRegister();

	VarRegister& operator=(const VarRegister &) { return *this; }

	// ================================================================================================================

	void addRegister(const Identifier &id, var newValue, VarTypeChecker::VarTypes expectedType=VarTypeChecker::Undefined);
	void setRegister(int registerIndex, var newValue);

	const var &getFromRegister(int registerIndex) const;
	int getRegisterIndex(const Identifier &id) const;
	int getNumUsedRegisters() const;
	Identifier getRegisterId(int index) const;
	const var *getVarPointer(int index) const;
	var *getVarPointer(int index);

    VarTypeChecker::VarTypes getRegisterVarType(int index) const
    {
        ignoreUnused(index);
        
#if ENABLE_SCRIPTING_SAFE_CHECKS
        if(isPositiveAndBelow(index, NUM_VAR_REGISTERS))
        {
            return registerTypes[index];
        }
#endif
        
        return VarTypeChecker::Undefined;
    }
    
	ReadWriteLock& getLock(int index);

private:

	// ================================================================================================================

	var registerStack[NUM_VAR_REGISTERS];
    
#if ENABLE_SCRIPTING_SAFE_CHECKS
    VarTypeChecker::VarTypes registerTypes[NUM_VAR_REGISTERS];
#endif
    
	Identifier registerStackIds[NUM_VAR_REGISTERS];
	ReadWriteLock registerLocks[NUM_VAR_REGISTERS];

	const var empty;

	// ================================================================================================================
};


#define NUM_API_FUNCTION_SLOTS 64

class ApiClass;



struct DiagnosticBase: public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<DiagnosticBase>;

    typedef var(*call0)(ApiClass*);
    typedef var(*call1)(ApiClass*, var);
    typedef var(*call2)(ApiClass*, var, var);
    typedef var(*call3)(ApiClass*, var, var, var);
    typedef var(*call4)(ApiClass*, var, var, var, var);
    typedef var(*call5)(ApiClass*, var, var, var, var, var);
    
    struct ShadowParseData
    {
        struct DependencyResolver;

        struct Item: public ReferenceCountedObject
        {
            enum class Type
            {
                onInit,
                StandaloneCode,
                ExternalFile,
                Callback
            };

            enum class CallbackFormat
            {
                PureId,
                IncludeParenthesis,
                ForceEmpty
            };

            using Ptr = ReferenceCountedObjectPtr<Item>;
            using List = ReferenceCountedArray<Item>;



            Item(Type t_, const String& fileName_, const String& code_) :
                t(t_),
                fileOrCallbackName(fileName_),
                code(code_)
            {}

            void resolveDependencies(DependencyResolver* r);

            String getFileName(CallbackFormat f=CallbackFormat::IncludeParenthesis) const 
            { 
                if (t == Type::ExternalFile)
                    return fileOrCallbackName;

                if (t == Type::Callback)
                {
                    switch (f)
                    {
                    case CallbackFormat::ForceEmpty: return "";
                    case CallbackFormat::PureId: return fileOrCallbackName;
                    case CallbackFormat::IncludeParenthesis: return fileOrCallbackName + "()";
                    default: jassertfalse; return "";

                    }
                }

                return "";
            }

            const Type t;
            
            const String code;

            Item* const* begin() const { return dependencies.begin(); }
            Item* const* end() const { return dependencies.end(); }

            void setBreakPosition(int pos)
            {
                stopAtLineNumber = pos;
            }

            int getBreakPosition() const { return stopAtLineNumber; }

        private:

            const String fileOrCallbackName;
            int stopAtLineNumber = -1;

            List dependencies;
        };

        struct DependencyResolver
        {
            DependencyResolver() = default;

            virtual ~DependencyResolver() {}
            virtual Item::List resolveDependencies(Item::Ptr item) = 0;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DependencyResolver);
        };

        Item::List items;
    };

#if USE_BACKEND
    struct DiagnosticResult
    {
        using QueryFunction = std::function<DiagnosticResult(DiagnosticBase*, const Identifier&, const Array<var>&)>;

        enum class Severity
        {
            OK,
            Unknown,
            Error,
            Warning,
            Info,
            Hint,
            numSeverity
        };

        enum class ErrorCode
        {
            Unknown = 0,

            // errors
            RuntimeIdFailure = 2001,
            CallbackArgMismatch = 2002,
            CallScopeMismatch = 2003,
            InvalidOption = 2004,
            SyntaxError = 2005,
            TypeMismatch = 2006,
            ArgMismatch = 2007,
            MissingFunctionOrProperty = 2008,
            InternalError = 2009,

            // warnings
            Deprecated = 3001,
            GraphicsIssue = 3002,
            UntouchedMethod = 3003,
            RangeCheck = 3004,

            // hints
            BestPractice = 5001
        };

        enum class Classification
        {
            Syntax,
            ApiValidation,
            TypeCheck,
            Language,
            CallScope,
            Deprecation
        };

        static String getClassificationString(Classification c)
        {
            switch (c)
            {
            case Classification::Syntax:
                return "syntax";
            case Classification::ApiValidation:
                return "api-validation";
            case Classification::TypeCheck:
                return "type-check";
            case Classification::Language:
                return "language";
            case Classification::CallScope:
                return "callscope";
            case Classification::Deprecation:
                return "deprecation";
            default:
                return "unknown";
            }
        }

        static String getSeverityString(Severity s)
        {
            switch (s)
            {
            case Severity::Error:   return "error";
            case Severity::Warning: return "warning";
            case Severity::Info:    return "info";
            case Severity::Hint:    return "hint";
            case Severity::Unknown: return "unknown";
            case Severity::OK:      return "ok";
            default:                return "error";
            }
        }

        struct CallStackItem
        {
            int line = 0;
            int col = 0;
            int charIndex = 0;
            String fileName;
            String methodName;
        };

        /** A small helper POD that carries over the data from a diagnostic event. */
        struct Item
        {
            int line = 0;
            int col = 0;
            int charIndex = 0;
            String fileName;
            String message;
            StringArray suggestions;
            Severity severity = Severity::Error;
            ErrorCode ec = ErrorCode::Unknown;

            std::vector<CallStackItem> callstack;

            DiagnosticResult::Classification classification;

            /** Formats the diagnostic matching F5 compile error output:
                "locationString: message {{Base64(processorId|path|charIndex|line|col)}}"
                This makes console output double-clickable and parseable by RestHelpers::parseError(). */
            String toConsoleString(Processor* p) const;

            String toEditorString() const;

        };

        DiagnosticResult(Severity s, const String& msg) :
            severity(s),
            message(msg)
        {
        }

        DiagnosticResult(const String& msg=String()) :
            severity(Severity::Unknown),
            message(msg)
        {}

        DiagnosticResult withSeverity(Severity s) const { auto copy = *this; copy.severity = s; return copy; }

        static DiagnosticResult ok() { return { Severity::OK, "" }; }
        static DiagnosticResult unknown(const String& msg={}) { return {Severity::Unknown, msg }; }
        static DiagnosticResult fail(const String& msg) { return { Severity::Error, msg }; }

        bool shouldReport() const { return severity > Severity::Unknown; }

        DiagnosticResult withMessage(const String& msg) { auto copy = *this; copy.message = msg; return copy; }

        DiagnosticResult withSuggestion(const String& s, bool clearOthers=false) const { auto copy = *this; if (clearOthers) copy.suggestions.clear(); copy.suggestions.add(s); return copy; }

        DiagnosticResult withErrorCode(ErrorCode c) const { auto copy = *this; copy.errorCode = c; return copy; }

        DiagnosticResult withFuzzySuggestion(const StringArray& availableTokens) const
        {
            auto copy = *this;
            jassert(wrongToken.isNotEmpty());

            auto sug = FuzzySearcher::suggestCorrection(wrongToken, availableTokens, 0.6);

            if (sug.isEmpty() && availableTokens.size() < 5)
            {
                copy.suggestions.addArray(availableTokens);
            }

            if(sug.isNotEmpty())
                copy.suggestions.add(sug);

            return copy;
        }

        DiagnosticResult withWrongToken(const String& wrongToken) const
        {
            auto copy = *this;
            copy.wrongToken = wrongToken.removeCharacters("`");
            return copy;
        }

        DiagnosticResult withExpectation(const String& expected, const String& actual) const
        {
            auto copy = *this;
            copy.wrongToken = actual;
            copy.rightToken = expected;
            return copy;
        }

        DiagnosticResult withClassification(Classification c) const
        {
            auto copy = *this;
            copy.source = c;
            return copy;
        }

        Severity getSeverity() const { return severity; }

        String getErrorMessage() const
        {
            String msg;

            msg << message;

            auto surroundWithBackticks = [](const String& s)
            {
                return "`" + s.removeCharacters("`") + "`";
            };

            if (wrongToken.isNotEmpty() && rightToken.isNotEmpty())
                msg << ": expected: " << surroundWithBackticks(rightToken) << ", actual: " << surroundWithBackticks(wrongToken);
            else if (wrongToken.isNotEmpty())
                msg << ": " << surroundWithBackticks(wrongToken);

            return msg;
        }
        StringArray getSuggestions() const { return suggestions; }
        Classification getClassification() const { return source; }
        ErrorCode getErrorCode() const { return errorCode; }

        /** Numeric priority for coalescing — higher value = more severe. */
        static int severityPriority(Severity s)
        {
            switch (s)
            {
            case Severity::OK:      return 0;
            case Severity::Hint:    return 1;
            case Severity::Info:    return 2;
            case Severity::Warning: return 3;
            case Severity::Error:   return 4;
            default:                return -1;
            }
        }

        /** Return the more severe of two results. */
        static DiagnosticResult max(const DiagnosticResult& a, const DiagnosticResult& b)
        {
            if (severityPriority(a.getSeverity()) >= severityPriority(b.getSeverity()))
                return a;
            return b;
        }

        /** Compose multiple diagnostic check functions into a single QueryFunction.
        Runs all checks and returns the result with the highest severity.
        Works with static functions, lambdas, and any callable matching the
        QueryFunction signature. */
        template <typename... Fns> static QueryFunction combine(Fns... checks)
        {
            return [=](DiagnosticBase* c, const Identifier& id, const Array<var>& args) -> DiagnosticResult
            {
                DiagnosticResult best = ok();
                ((best = DiagnosticResult::max(best, checks(c, id, args))), ...);
                return best;
            };
        }

        /** Appends a callstack item to the callstack. */
        DiagnosticResult withCallStackItem(const CallStackItem& item) const
        {
            auto copy = *this;
            copy.callstack.push_back(item);
            return copy;
        }

    private:

        std::vector<CallStackItem> callstack;
        Severity severity;
        ErrorCode errorCode = ErrorCode::Unknown;
        Classification source = Classification::ApiValidation;
        String message;
        String wrongToken;
        String rightToken;
        StringArray suggestions;

    };
    
    struct ShadowParserBase
    {
        virtual ~ShadowParserBase() {};

        virtual void addDiagnostic(const DiagnosticResult::Item& r) = 0;

        Identifier currentNamespace;
    };

    virtual var getReturnType(const Identifier& fn, const Array<var>& args) const
    {
        jassertfalse;
        return {};
    };
    
    void markMethodTouched(const Identifier& methodName)
    {
        touchedMethods.insert(methodName);
    }

    bool wasMethodTouched(const Identifier& methodName) const
    {
        return touchedMethods.count(methodName) > 0;
    }

    void clearTouchedMethods()
    {
        touchedMethods.clear();
    }

    /** Registers a diagnostic function that is evaluated at parse time.

        A diagnostic function is a lambda that performs dynamic checks on the validity of a API call.
        What this check is implementing is completely up to the call site. It is used by the ApiValidationAnalyzer
        in the shadow parser to collect dynamic diagnostic reports for the LSP server.

        QueryFunction must be a callable object with this signature:

        DiagnosticResult getDiagnostics(DiagnosticBase* c, const Array<var>& arguments>)

        Note that the arguments object might contain undefined values for parameter values that
        could not be deduced at parse time. If you rely on that check, then just return
        DiagnosticResult::unknown().
    */
    virtual void addDiagnostic(const Identifier& methodName, const DiagnosticResult::QueryFunction& qf)
    {
        diagnostics[methodName] = qf;
    }
    

    /** Performs the assigned diagnostic check in the parser. */
    DiagnosticResult performDiagnostic(const Identifier& methodName, const Array<var>& args)
    {
        jassert(hasDiagnosticCheck(methodName));
        return diagnostics.at(methodName)(this, methodName, args);
    }

    virtual DiagnosticResult checkFunction(const Identifier& methodName, int expectedNumArgs) const { return DiagnosticResult::unknown(); }

    /** Used by the parser to check whether to evaluate the arguments for the check. */
    bool hasDiagnosticCheck(const Identifier& methodName) const
    {
        return diagnostics.find(methodName) != diagnostics.end();
    }
    
    std::map<Identifier, DiagnosticResult::QueryFunction> diagnostics;
    std::set<Identifier> touchedMethods;
    
#endif
    
    virtual DiagnosticResult checkArgCount(const Identifier& id, int numActual) const
    {
        int numExpected;
        int index;
        
        if (getIndexAndNumArgsForFunction(id, index, numExpected))
        {
            if (numExpected != numActual)
            {
                String msg;
                msg << "expects " << numExpected << " argument"
                    << (numExpected != 1 ? "s" : "")
                    << ", got " << numActual;

                return DiagnosticResult::fail(msg);
            }
        }

        return DiagnosticResult::ok();
    }

    virtual bool getIndexAndNumArgsForFunction(const Identifier &id, int &index, int &numArgs) const = 0;
    
    virtual ~DiagnosticBase() {};
    
    virtual void addFunction(const Identifier &id, call0 newFunction) = 0;
    virtual void addFunction1(const Identifier &id, call1 newFunction) = 0;
    virtual void addFunction2(const Identifier &id, call2 newFunction) = 0;
    virtual void addFunction3(const Identifier &id, call3 newFunction) = 0;
    virtual void addFunction4(const Identifier &id, call4 newFunction) = 0;
    virtual void addFunction5(const Identifier &id, call5 newFunction) = 0;
    
    ShadowParserBase* currentShadowParser;
};






/** A API class is a class with a fixed number of methods and constants that can be called from Javascript.
*
*	It is used to improve the performance of calling C++ functions from Javascript. 
*   This is achieved by resolving the function call at compile time making the call from Javascript almost as fast 
*   as a regular C++ function call (the arguments still need to be evaluated).
*
*	You can also define constants which are resolved into literal values at compile time making them exactly 
*   as fast as typing the literal value.
*
*	A ApiClass needs to be registered on the C++ side before the ScriptingEngine parses and executes the code using
*   HiseJavascriptEngine::registerApiClass().
*
*   To use this class, subclass it, write the methods you want to expose to Javascript as member functions of your
*   subclass and use these two macros to publish them to the Javascript Engine:
*
 *      @code{.cpp}
 *		class MyApiClass: public ApiClass
 *		{
 *		public:
 *		   MyApiClass():
 *		     ApiClass(0) // we don't need constants here...
 *		   {
 *		       ADD_API_METHOD_1(square); // This adds the method wrapper to the function list
 *		   }
 *
 *
 *		   double square(double x) const
 *		   {
 *		       return x*x;
 *		   };
 *
 *		   struct Wrapper // You'll need a subclass called `Wrapper` for it to work...
 *		   {
 *		   		// this defines a wrapper function that is using a static_cast to call the member function.
 *		 		ADD_API_METHOD_WRAPPER_1(MyApiClass, square);
 *		   };
 *		};
        @endcode
 *
 *  The Macros support up to 5 arguments. You'll need another macro for void functions: `API_VOID_METHOD_WRAPPER_X`,
 *  but apart from this, it is pretty straight forward...
 *
 *  For a living example of an API class take a look at eg. the Math class or the ScriptingApi::Engine class.
 */
class ApiClass : public DiagnosticBase,
				 public DebugableObjectBase
{
public:

	// ================================================================================================================

	

	// ================================================================================================================

    /** Creates a Api class with the given amount of constants.
    *
    *   If numConstants_ is not zero, you'll need to add the constants in your subclass constructor using addConstant().
    *   You also need to register every function in your class there.
    */
	ApiClass(int numConstants_);;
	virtual ~ApiClass();

	/** You can overwrite this method and return true if you want to allow illegal calls that would otherwise
	*	fire a warning. This is eg. used in the Console class to prevent firing when debugging. */
	virtual bool allowIllegalCallsOnAudioThread(int /*functionIndex*/) const { return false; }

	String getDebugName() const override { return getObjectName().toString(); }

	String getDebugValue() const override { return ""; }

	bool isWatchable() const override { return false; };

	// ================================================================================================================

    /** Adds a constant. You can give it a name (it must be a valid Identifier) and a value and will be resolved at
    *   compile time to accelerate the access to it. */
	void addConstant(String constantName, var value);
    
    /** Returns the constant at the given index. */
	const var getConstantValue(int index) const;
    
    /** Return the index for the given name. */
	int getConstantIndex(const Identifier &id) const;

	/** Returns the name for the constant as it is used in the scripting context. */
	Identifier getConstantName(int index) const;

	// ================================================================================================================

    /** Adds a function with no parameters. 
    *
    *   You don't need to use this directly, but use the macro ADD_API_METHOD_0() for it. */
	void addFunction(const Identifier &id, call0 newFunction) override;;
	
    /** Adds a function with one parameter.
     *
     *   You don't need to use this directly, but use the macro ADD_API_METHOD_1() for it. */
    void addFunction1(const Identifier &id, call1 newFunction) override;
	
    /** Adds a function with two parameters.
     *
     *   You don't need to use this directly, but use the macro ADD_API_METHOD_2() for it. */
    void addFunction2(const Identifier &id, call2 newFunction) override;
	
    /** Adds a function with three parameters.
     *
     *   You don't need to use this directly, but use the macro ADD_API_METHOD_3() for it. */
    void addFunction3(const Identifier &id, call3 newFunction) override;
	
    /** Adds a function with four parameters.
     *
     *   You don't need to use this directly, but use the macro ADD_API_METHOD_4() for it. */
    void addFunction4(const Identifier &id, call4 newFunction) override;
	
    /** Adds a function with five parameters.
     *
     *   You don't need to use this directly, but use the macro ADD_API_METHOD_5() for it. */
    void addFunction5(const Identifier &id, call5 newFunction) override;

#if ENABLE_SCRIPTING_SAFE_CHECKS
    void addForcedParameterTypes(const Identifier& id, const VarTypeChecker::ParameterTypes& types)
    {
        ForcedParameterInfo info;
        
        info.id = id;
        info.types = types;
        
        getIndexAndNumArgsForFunction(id, info.index, info.numArgs);
        forcedParameterTypes.add(info);
    }
#endif
    
    VarTypeChecker::ParameterTypes getForcedParameterTypes(int index, int numArgs) const
    {
#if ENABLE_SCRIPTING_SAFE_CHECKS
        for(const auto& t: forcedParameterTypes)
        {
            if(t.index == index && t.numArgs == numArgs)
                return t.types;
        }
#endif
        
        return {};

    }
    
    VarTypeChecker::ParameterTypes getForcedParameterTypes(const Identifier& id) const
    {
#if ENABLE_SCRIPTING_SAFE_CHECKS
        for(const auto& t: forcedParameterTypes)
        {
            if(t.id == id)
                return t.types;
        }
#endif
        
        return {};
    }
    
    /** This will fill in the information for the given function. 
    *
    *   The JavascriptEngine uses this to resolve the function call into a function pointer at compile time.
    *   When the script is executed, this information will be used for blazing fast access to the methods.*/
	bool getIndexAndNumArgsForFunction(const Identifier &id, int &index, int &numArgs) const override;
    
    /** Calls the function with the index and the argument data.
    *
    *   You'll need to call getIndexAndNumArgsForFunction() before calling this. */
	var callFunction(int index, var *args, int numArgs);

    /** This returns all function names alphabetically sorted. This is used by the autocomplete popup. */
	void getAllFunctionNames(Array<Identifier> &ids) const;
    
    /** Returns all constant names as alphabetically sorted array. This is used by the autocomplete popup. */
	void getAllConstants(Array<Identifier> &ids) const;

	ReadWriteLock apiClassLock;

	int getNumChildElements() const override { return numConstants; }
	

    
	DebugInformationBase* getChildElement(int index) override
	{
		auto name = getConstantName(index);
		auto s = new SettableDebugInfo();
		s->codeToInsert << "%PARENT%." << name;
		s->value = getConstantValue(index);
		s->watchable = false;
		s->autocompleteable = false;
		return s;
	}


	bool isAutocompleteable() const override { return true; }

	void setFunctionIsInlineable(const Identifier& id)
	{
		inlineableFunctions.add(id);
	}

	bool isInlineableFunction(const Identifier& id) const
	{
		return inlineableFunctions.contains(id);
	}

	var getListOfOptimizableFunctions() const
	{
		Array<var> l;

		for (auto o : optimizableFunctions)
		{
			if (o != nullptr)
				l.add(var(dynamic_cast<ReferenceCountedObject*>(o.get())));
		}

		return var(l);
	}

	void addOptimizableFunction(const var& functionObject)
	{
		if (auto obj = dynamic_cast<DebugableObjectBase*>(functionObject.getObject()))
		{
			optimizableFunctions.addIfNotAlreadyThere(obj);
		}
	}

	DebugableObjectBase::Location getCurrentLocationInFunctionCall()
	{
		return currentLocation;
	}

	void setWantsCurrentLocation(bool shouldSaveCurrentLocation)
	{
		wantsLocation = shouldSaveCurrentLocation;
	}

	bool wantsCurrentLocation() const { return wantsLocation; }

	void setCurrentLocation(const String& file, int charNumber)
	{
		currentLocation.fileName = file;
		currentLocation.charNumber = charNumber;
	}

#if USE_BACKEND
    void addDiagnostic(const Identifier& methodName, const DiagnosticResult::QueryFunction& qf) override
    {
        int unused;
        auto ok = getIndexAndNumArgsForFunction(methodName, unused, unused);

        // if you hit this you need to add the function first...
        jassert(ok);
        ignoreUnused(ok);
        
        DiagnosticBase::addDiagnostic(methodName, qf);
    }
#endif

private:

	bool wantsLocation = false;
	DebugableObjectBase::Location currentLocation;


	Array<WeakReference<DebugableObjectBase>> optimizableFunctions;

	// ================================================================================================================

    static constexpr int NumMaxArguments = 6;
    static constexpr int NumSlots = NUM_API_FUNCTION_SLOTS;

    std::array<std::array<Identifier, NumSlots>, NumMaxArguments> ids;
    std::array<std::array<void*, NumSlots>, NumMaxArguments> functions;

    template <int NumArgs> void addFunctionT(const Identifier &id, void* newFunction)
	{
#if JUCE_DEBUG
        for(auto& idList: ids)
        {
	        for(auto& existingId: idList)
	        {
		        if(existingId == id)
		        {
			        jassertfalse;
		        }
	        }
        }
#endif


		for(int i = 0; i < NumSlots; i++)
		{
			if(functions[NumArgs][i] == nullptr)
			{
				functions[NumArgs][i] = newFunction;
				ids[NumArgs][i] = id;
				return;
			}
		}
		
		jassertfalse;
	}

#if ENABLE_SCRIPTING_SAFE_CHECKS
    struct ForcedParameterInfo
    {
        int numArgs;
        int index;
        Identifier id;
        VarTypeChecker::ParameterTypes types;
    };
    
    Array<ForcedParameterInfo> forcedParameterTypes;
#endif
    
	// ================================================================================================================

	struct Constant
	{
		Constant(const Identifier &id_, var value_);
		Constant();
		Constant& operator=(const Constant& other);

		static Constant null;
		Identifier id;
		var value;
	};

	Constant constants[8];
	const int numConstants;

	Constant* constantsToUse;

	Array<Constant> constantBigStorage;

	Array<Identifier> inlineableFunctions;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApiClass)

	// ================================================================================================================
};

} // namespace hise
#endif  // JAVASCRIPTAPICLASS_H_INCLUDED
