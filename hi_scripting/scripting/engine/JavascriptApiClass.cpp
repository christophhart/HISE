
namespace hise { using namespace juce;

Identifier VarTypeChecker::getTypeName(TypedObject to)
{
	switch (to)
	{
	case TypedObject::AudioFile: return VarTypeIdentifiers::AudioFile;
	case TypedObject::AudioSampleProcessor: return VarTypeIdentifiers::AudioSampleProcessor;
	case TypedObject::BackgroundTask: return VarTypeIdentifiers::BackgroundTask;
	case TypedObject::BeatportManager: return VarTypeIdentifiers::BeatportManager;
	case TypedObject::Broadcaster: return VarTypeIdentifiers::Broadcaster;
	case TypedObject::Buffer: return VarTypeIdentifiers::Buffer;
	case TypedObject::Builder: return VarTypeIdentifiers::Builder;
	case TypedObject::ChildSynth: return VarTypeIdentifiers::ChildSynth;
	case TypedObject::ComplexGroupManager: return VarTypeIdentifiers::ComplexGroupManager;
	case TypedObject::Connection: return VarTypeIdentifiers::Connection;
	case TypedObject::ContainerChild: return VarTypeIdentifiers::ContainerChild;
	case TypedObject::DisplayBuffer: return VarTypeIdentifiers::DisplayBuffer;
	case TypedObject::DisplayBufferSource: return VarTypeIdentifiers::DisplayBufferSource;
	case TypedObject::Download: return VarTypeIdentifiers::Download;
	case TypedObject::DspModule: return VarTypeIdentifiers::DspModule;
	case TypedObject::DspNetwork: return VarTypeIdentifiers::DspNetwork;
	case TypedObject::Effect: return VarTypeIdentifiers::Effect;
	case TypedObject::ErrorHandler: return VarTypeIdentifiers::ErrorHandler;
	case TypedObject::Expansion: return VarTypeIdentifiers::Expansion;
	case TypedObject::ExpansionHandler: return VarTypeIdentifiers::ExpansionHandler;
	case TypedObject::FFT: return VarTypeIdentifiers::FFT;
	case TypedObject::File: return VarTypeIdentifiers::File;
	case TypedObject::FixObjectArray: return VarTypeIdentifiers::FixObjectArray;
	case TypedObject::FixObjectFactory: return VarTypeIdentifiers::FixObjectFactory;
	case TypedObject::FixObjectStack: return VarTypeIdentifiers::FixObjectStack;
	case TypedObject::GlobalCable: return VarTypeIdentifiers::GlobalCable;
	case TypedObject::GlobalRoutingManager: return VarTypeIdentifiers::GlobalRoutingManager;
	case TypedObject::Graphics: return VarTypeIdentifiers::Graphics;
	case TypedObject::LorisManager: return VarTypeIdentifiers::LorisManager;
	case TypedObject::MacroHandler: return VarTypeIdentifiers::MacroHandler;
	case TypedObject::MarkdownRenderer: return VarTypeIdentifiers::MarkdownRenderer;
	case TypedObject::MessageHolder: return VarTypeIdentifiers::MessageHolder;
	case TypedObject::MidiAutomationHandler: return VarTypeIdentifiers::MidiAutomationHandler;
	case TypedObject::MidiList: return VarTypeIdentifiers::MidiList;
	case TypedObject::MidiPlayer: return VarTypeIdentifiers::MidiPlayer;
	case TypedObject::MidiProcessor: return VarTypeIdentifiers::MidiProcessor;
	case TypedObject::Modifiers: return VarTypeIdentifiers::Modifiers;
	case TypedObject::Modulator: return VarTypeIdentifiers::Modulator;
	case TypedObject::NetworkTest: return VarTypeIdentifiers::NetworkTest;
	case TypedObject::NeuralNetwork: return VarTypeIdentifiers::NeuralNetwork;
	case TypedObject::Node: return VarTypeIdentifiers::Node;
	case TypedObject::Parameter: return VarTypeIdentifiers::Parameter;
	case TypedObject::Path: return VarTypeIdentifiers::Path;
	case TypedObject::Rectangle: return VarTypeIdentifiers::Rectangle;
	case TypedObject::RoutingMatrix: return VarTypeIdentifiers::RoutingMatrix;
	case TypedObject::Sample: return VarTypeIdentifiers::Sample;
	case TypedObject::ScriptAudioWaveform: return VarTypeIdentifiers::ScriptAudioWaveform;
	case TypedObject::ScriptButton: return VarTypeIdentifiers::ScriptButton;
	case TypedObject::ScriptComboBox: return VarTypeIdentifiers::ScriptComboBox;
	case TypedObject::ScriptDynamicContainer: return VarTypeIdentifiers::ScriptDynamicContainer;
	case TypedObject::ScriptFloatingTile: return VarTypeIdentifiers::ScriptFloatingTile;
	case TypedObject::ScriptImage: return VarTypeIdentifiers::ScriptImage;
	case TypedObject::ScriptLabel: return VarTypeIdentifiers::ScriptLabel;
	case TypedObject::ScriptLookAndFeel: return VarTypeIdentifiers::ScriptLookAndFeel;
	case TypedObject::ScriptModulationMatrix: return VarTypeIdentifiers::ScriptModulationMatrix;
	case TypedObject::ScriptMultipageDialog: return VarTypeIdentifiers::ScriptMultipageDialog;
	case TypedObject::ScriptPanel: return VarTypeIdentifiers::ScriptPanel;
	case TypedObject::ScriptShader: return VarTypeIdentifiers::ScriptShader;
	case TypedObject::ScriptSlider: return VarTypeIdentifiers::ScriptSlider;
	case TypedObject::ScriptSliderPack: return VarTypeIdentifiers::ScriptSliderPack;
	case TypedObject::ScriptTable: return VarTypeIdentifiers::ScriptTable;
	case TypedObject::ScriptWebView: return VarTypeIdentifiers::ScriptWebView;
	case TypedObject::ScriptedViewport: return VarTypeIdentifiers::ScriptedViewport;
	case TypedObject::SliderPackData: return VarTypeIdentifiers::SliderPackData;
	case TypedObject::SliderPackProcessor: return VarTypeIdentifiers::SliderPackProcessor;
	case TypedObject::SlotFX: return VarTypeIdentifiers::SlotFX;
	case TypedObject::Table: return VarTypeIdentifiers::Table;
	case TypedObject::TableProcessor: return VarTypeIdentifiers::TableProcessor;
	case TypedObject::ThreadSafeStorage: return VarTypeIdentifiers::ThreadSafeStorage;
	case TypedObject::Timer: return VarTypeIdentifiers::Timer;
	case TypedObject::TransportHandler: return VarTypeIdentifiers::TransportHandler;
	case TypedObject::Unlocker: return VarTypeIdentifiers::Unlocker;
	case TypedObject::UnorderedStack: return VarTypeIdentifiers::UnorderedStack;
	case TypedObject::UserPresetHandler: return VarTypeIdentifiers::UserPresetHandler;
	case TypedObject::WavetableController: return VarTypeIdentifiers::WavetableController;
	default: jassertfalse; return {};
	}
}

Identifier VarTypeChecker::getTypeName(VarTypes type)
{
	switch (type)
	{
	case Undefined: return VarTypeIdentifiers::Undefined;
	case Integer: return VarTypeIdentifiers::Integer;
	case Double: return VarTypeIdentifiers::Double;
	case Number: return VarTypeIdentifiers::Number;
	case String: return VarTypeIdentifiers::String;
	case Colour: return VarTypeIdentifiers::Colour;
	case Array: return VarTypeIdentifiers::Array;
	case IndexOrArray: return VarTypeIdentifiers::IndexOrArray;
	case AudioData: return VarTypeIdentifiers::AudioData;
	case Buffer: return VarTypeIdentifiers::Buffer;
	case ObjectWithLength: return VarTypeIdentifiers::ObjectWithLength;
	case JSON: return VarTypeIdentifiers::JSON;
	case ScriptObject: return VarTypeIdentifiers::ScriptObject;
	case Object: return VarTypeIdentifiers::Object;
	case Function: return VarTypeIdentifiers::Function;
	case ComplexType: return VarTypeIdentifiers::ComplexType;
	case NotUndefined: return VarTypeIdentifiers::NotUndefined;
	default: jassertfalse; return "Unknown";
	}
}

VarTypeChecker::WrappedType VarTypeChecker::getTypeFromString(const juce::Identifier& name)
{
	if (name == VarTypeIdentifiers::Undefined)			return WrappedType(VarTypes::Undefined);
	if (name == VarTypeIdentifiers::Integer)			return WrappedType(VarTypes::Integer);
	if (name == VarTypeIdentifiers::Double)				return WrappedType(VarTypes::Double);
	if (name == VarTypeIdentifiers::Number)				return WrappedType(VarTypes::Number);
	if (name == VarTypeIdentifiers::String)				return WrappedType(VarTypes::String);
	if (name == VarTypeIdentifiers::Colour)				return WrappedType(VarTypes::Colour);
	if (name == VarTypeIdentifiers::IndexOrArray)		return WrappedType(VarTypes::IndexOrArray);
	if (name == VarTypeIdentifiers::Array)				return WrappedType(VarTypes::Array);
	if (name == VarTypeIdentifiers::AudioData)			return WrappedType(VarTypes::AudioData);
	if (name == VarTypeIdentifiers::Buffer)				return WrappedType(VarTypes::Buffer);
	if (name == VarTypeIdentifiers::ObjectWithLength)	return WrappedType(VarTypes::ObjectWithLength);
	if (name == VarTypeIdentifiers::JSON)				return WrappedType(VarTypes::JSON);
	if (name == VarTypeIdentifiers::ScriptObject)		return WrappedType(VarTypes::ScriptObject);
	if (name == VarTypeIdentifiers::Object)				return WrappedType(VarTypes::Object);
	if (name == VarTypeIdentifiers::Function)			return WrappedType(VarTypes::Function);
	if (name == VarTypeIdentifiers::ComplexType)		return WrappedType(VarTypes::ComplexType);
	if (name == VarTypeIdentifiers::NotUndefined)		return WrappedType(VarTypes::NotUndefined);

	if(name == VarTypeIdentifiers::AudioFile)				 return WrappedType(TypedObject::AudioFile);
	if(name == VarTypeIdentifiers::AudioSampleProcessor)	 return WrappedType(TypedObject::AudioSampleProcessor);
	if(name == VarTypeIdentifiers::BackgroundTask)			 return WrappedType(TypedObject::BackgroundTask);
	if(name == VarTypeIdentifiers::BeatportManager)			 return WrappedType(TypedObject::BeatportManager);
	if(name == VarTypeIdentifiers::Broadcaster)				 return WrappedType(TypedObject::Broadcaster);
	if(name == VarTypeIdentifiers::Buffer)					 return WrappedType(TypedObject::Buffer);
	if(name == VarTypeIdentifiers::Builder)					 return WrappedType(TypedObject::Builder);
	if(name == VarTypeIdentifiers::ChildSynth)				 return WrappedType(TypedObject::ChildSynth);
	if(name == VarTypeIdentifiers::ComplexGroupManager)		 return WrappedType(TypedObject::ComplexGroupManager);
	if(name == VarTypeIdentifiers::Connection)				 return WrappedType(TypedObject::Connection);
	if(name == VarTypeIdentifiers::ContainerChild)			 return WrappedType(TypedObject::ContainerChild);
	if(name == VarTypeIdentifiers::DisplayBuffer)			 return WrappedType(TypedObject::DisplayBuffer);
	if(name == VarTypeIdentifiers::DisplayBufferSource)		 return WrappedType(TypedObject::DisplayBufferSource);
	if(name == VarTypeIdentifiers::Download)				 return WrappedType(TypedObject::Download);
	if(name == VarTypeIdentifiers::DspModule)				 return WrappedType(TypedObject::DspModule);
	if(name == VarTypeIdentifiers::DspNetwork)				 return WrappedType(TypedObject::DspNetwork);
	if(name == VarTypeIdentifiers::Effect)					 return WrappedType(TypedObject::Effect);
	if(name == VarTypeIdentifiers::ErrorHandler)			 return WrappedType(TypedObject::ErrorHandler);
	if(name == VarTypeIdentifiers::Expansion)				 return WrappedType(TypedObject::Expansion);
	if(name == VarTypeIdentifiers::ExpansionHandler)		 return WrappedType(TypedObject::ExpansionHandler);
	if(name == VarTypeIdentifiers::FFT)						 return WrappedType(TypedObject::FFT);
	if(name == VarTypeIdentifiers::File)					 return WrappedType(TypedObject::File);
	if(name == VarTypeIdentifiers::FixObjectArray)			 return WrappedType(TypedObject::FixObjectArray);
	if(name == VarTypeIdentifiers::FixObjectFactory)		 return WrappedType(TypedObject::FixObjectFactory);
	if(name == VarTypeIdentifiers::FixObjectStack)			 return WrappedType(TypedObject::FixObjectStack);
	if(name == VarTypeIdentifiers::GlobalCable)				 return WrappedType(TypedObject::GlobalCable);
	if(name == VarTypeIdentifiers::GlobalRoutingManager)	 return WrappedType(TypedObject::GlobalRoutingManager);
	if(name == VarTypeIdentifiers::Graphics)				 return WrappedType(TypedObject::Graphics);
	if(name == VarTypeIdentifiers::LorisManager)			 return WrappedType(TypedObject::LorisManager);
	if(name == VarTypeIdentifiers::MacroHandler)			 return WrappedType(TypedObject::MacroHandler);
	if(name == VarTypeIdentifiers::MarkdownRenderer)		 return WrappedType(TypedObject::MarkdownRenderer);
	if(name == VarTypeIdentifiers::MessageHolder)			 return WrappedType(TypedObject::MessageHolder);
	if(name == VarTypeIdentifiers::MidiAutomationHandler)	 return WrappedType(TypedObject::MidiAutomationHandler);
	if(name == VarTypeIdentifiers::MidiList)				 return WrappedType(TypedObject::MidiList);
	if(name == VarTypeIdentifiers::MidiPlayer)				 return WrappedType(TypedObject::MidiPlayer);
	if(name == VarTypeIdentifiers::MidiProcessor)			 return WrappedType(TypedObject::MidiProcessor);
	if(name == VarTypeIdentifiers::Modifiers)				 return WrappedType(TypedObject::Modifiers);
	if(name == VarTypeIdentifiers::Modulator)				 return WrappedType(TypedObject::Modulator);
	if(name == VarTypeIdentifiers::NetworkTest)				 return WrappedType(TypedObject::NetworkTest);
	if(name == VarTypeIdentifiers::NeuralNetwork)			 return WrappedType(TypedObject::NeuralNetwork);
	if(name == VarTypeIdentifiers::Node)					 return WrappedType(TypedObject::Node);
	if(name == VarTypeIdentifiers::Parameter)				 return WrappedType(TypedObject::Parameter);
	if(name == VarTypeIdentifiers::Path)					 return WrappedType(TypedObject::Path);
	if(name == VarTypeIdentifiers::Rectangle)				 return WrappedType(TypedObject::Rectangle);
	if(name == VarTypeIdentifiers::RoutingMatrix)			 return WrappedType(TypedObject::RoutingMatrix);
	if(name == VarTypeIdentifiers::Sample)					 return WrappedType(TypedObject::Sample);
	if(name == VarTypeIdentifiers::ScriptAudioWaveform)		 return WrappedType(TypedObject::ScriptAudioWaveform);
	if(name == VarTypeIdentifiers::ScriptButton)			 return WrappedType(TypedObject::ScriptButton);
	if(name == VarTypeIdentifiers::ScriptComboBox)			 return WrappedType(TypedObject::ScriptComboBox);
	if(name == VarTypeIdentifiers::ScriptDynamicContainer)	 return WrappedType(TypedObject::ScriptDynamicContainer);
	if(name == VarTypeIdentifiers::ScriptFloatingTile)		 return WrappedType(TypedObject::ScriptFloatingTile);
	if(name == VarTypeIdentifiers::ScriptImage)				 return WrappedType(TypedObject::ScriptImage);
	if(name == VarTypeIdentifiers::ScriptLabel)				 return WrappedType(TypedObject::ScriptLabel);
	if(name == VarTypeIdentifiers::ScriptLookAndFeel)		 return WrappedType(TypedObject::ScriptLookAndFeel);
	if(name == VarTypeIdentifiers::ScriptModulationMatrix)	 return WrappedType(TypedObject::ScriptModulationMatrix);
	if(name == VarTypeIdentifiers::ScriptMultipageDialog)	 return WrappedType(TypedObject::ScriptMultipageDialog);
	if(name == VarTypeIdentifiers::ScriptPanel)				 return WrappedType(TypedObject::ScriptPanel);
	if(name == VarTypeIdentifiers::ScriptShader)			 return WrappedType(TypedObject::ScriptShader);
	if(name == VarTypeIdentifiers::ScriptSlider)			 return WrappedType(TypedObject::ScriptSlider);
	if(name == VarTypeIdentifiers::ScriptSliderPack)		 return WrappedType(TypedObject::ScriptSliderPack);
	if(name == VarTypeIdentifiers::ScriptTable)				 return WrappedType(TypedObject::ScriptTable);
	if(name == VarTypeIdentifiers::ScriptWebView)			 return WrappedType(TypedObject::ScriptWebView);
	if(name == VarTypeIdentifiers::ScriptedViewport)		 return WrappedType(TypedObject::ScriptedViewport);
	if(name == VarTypeIdentifiers::SliderPackData)			 return WrappedType(TypedObject::SliderPackData);
	if(name == VarTypeIdentifiers::SliderPackProcessor)		 return WrappedType(TypedObject::SliderPackProcessor);
	if(name == VarTypeIdentifiers::SlotFX)					 return WrappedType(TypedObject::SlotFX);
	if(name == VarTypeIdentifiers::Table)					 return WrappedType(TypedObject::Table);
	if(name == VarTypeIdentifiers::TableProcessor)			 return WrappedType(TypedObject::TableProcessor);
	if(name == VarTypeIdentifiers::ThreadSafeStorage)		 return WrappedType(TypedObject::ThreadSafeStorage);
	if(name == VarTypeIdentifiers::Timer)					 return WrappedType(TypedObject::Timer);
	if(name == VarTypeIdentifiers::TransportHandler)		 return WrappedType(TypedObject::TransportHandler);
	if(name == VarTypeIdentifiers::Unlocker)				 return WrappedType(TypedObject::Unlocker);
	if(name == VarTypeIdentifiers::UnorderedStack)			 return WrappedType(TypedObject::UnorderedStack);
	if(name == VarTypeIdentifiers::UserPresetHandler)		 return WrappedType(TypedObject::UserPresetHandler);
	if(name == VarTypeIdentifiers::WavetableController)		 return WrappedType(TypedObject::WavetableController);

	return WrappedType();
}

VarTypeChecker::WrappedType VarTypeChecker::getType(const var& value)
{
    if(value.isInt() || value.isInt64() || value.isBool())
        return WrappedType(VarTypes::Integer);
    if(value.isDouble())
        return WrappedType(VarTypes::Double);
    if(value.isString())
        return WrappedType(VarTypes::String);
    if(value.isBuffer())
        return WrappedType(VarTypes::Buffer);
    if(value.isArray())
        return WrappedType(VarTypes::Array);
	if(HiseJavascriptEngine::isJavascriptFunction(value))
        return WrappedType(VarTypes::Function);
    if(value.getDynamicObject() != nullptr)
        return WrappedType(VarTypes::JSON);
	if (value.isObject())
	{
		if (auto b = dynamic_cast<LightweightDiagnostics*>(value.getObject()))
		{
			return getTypeFromString(b->getObjectName());
		}

		return WrappedType(VarTypes::ScriptObject);
	}
    
	return WrappedType();
}

VarRegister::VarRegister() :
empty(var())
{
	for (int i = 0; i < NUM_VAR_REGISTERS; i++)
	{
		registerStack[i] = var();
		registerStackIds[i] = Identifier();
        
#if ENABLE_SCRIPTING_SAFE_CHECKS
        registerTypes[i] = VarTypeChecker::Undefined;
#endif
	}
}

VarRegister::VarRegister(VarRegister &other) :
empty(var())
{
	for (int i = 0; i < NUM_VAR_REGISTERS; i++)
	{
		registerStack[i] = other.registerStack[i];
		registerStackIds[i] = other.registerStackIds[i];
	}
}

VarRegister::~VarRegister()
{
	for (int i = 0; i < NUM_VAR_REGISTERS; i++)
	{
		registerStack[i] = var();
		registerStackIds[i] = Identifier();
	}
}

void VarRegister::addRegister(const Identifier &id, var newValue, VarTypeChecker::VarTypes expectedType)
{
	for (int i = 0; i < NUM_VAR_REGISTERS; i++)
	{
		if (registerStackIds[i] == id)
		{
			registerStack[i] = newValue;
            
#if ENABLE_SCRIPTING_SAFE_CHECKS
            if(!registerTypes[i])
                registerTypes[i] = expectedType;
            else
            {
                auto ok = VarTypeChecker::checkType(newValue, VarTypeChecker::WrappedType(registerTypes[i]), true);
                
                if(ok.failed())
                    throw ok.getErrorMessage();
            }
                
#endif
            
			break;
		}

		if (registerStackIds[i].isNull())
		{
            registerStackIds[i] = id;
            
#if ENABLE_SCRIPTING_SAFE_CHECKS
            registerTypes[i] = expectedType;
#endif
            
			registerStack[i] = newValue;
			break;
		}
	}
}

void VarRegister::setRegister(int registerIndex, var newValue)
{
	if (registerIndex < NUM_VAR_REGISTERS)
	{
#if ENABLE_SCRIPTING_SAFE_CHECKS
        if(registerTypes[registerIndex])
        {
            auto ok = VarTypeChecker::checkType(newValue, VarTypeChecker::WrappedType(registerTypes[registerIndex]), true);
            
            if(ok.failed())
                throw ok.getErrorMessage();
        }
#endif
        
		registerStack[registerIndex] = newValue;
	}
}

const var & VarRegister::getFromRegister(int registerIndex) const
{
	if (registerIndex < NUM_VAR_REGISTERS)
	{
		return *(registerStack + registerIndex);
	}

	return empty;
}

int VarRegister::getRegisterIndex(const Identifier &id) const
{
	for (int i = 0; i < NUM_VAR_REGISTERS; i++)
	{
		if (registerStackIds[i] == id) return i;
	}

	return -1;
}

int VarRegister::getNumUsedRegisters() const
{
	for (int i = 0; i < NUM_VAR_REGISTERS; i++)
	{
		if (registerStackIds[i].isNull()) return i;
	}

	return NUM_VAR_REGISTERS;
}

Identifier VarRegister::getRegisterId(int index) const
{
	if (index < NUM_VAR_REGISTERS)
		return registerStackIds[index];

    return {};
}

const var * VarRegister::getVarPointer(int index) const
{

	if (index < NUM_VAR_REGISTERS)
		return registerStack + index;

	return nullptr;
}

var * VarRegister::getVarPointer(int index)
{

	if (index < NUM_VAR_REGISTERS)
		return registerStack + index;

	return nullptr;
}

ReadWriteLock& VarRegister::getLock(int index)
{
	return registerLocks[index];
}

ApiClass::ApiClass(int numConstants_) :
numConstants(numConstants_)
{
	for(auto& flist: functions)
	{
		for(auto& f: flist)
			f = nullptr;
	}
	
	if (numConstants > 8)
	{
		constantBigStorage = Array<Constant>();
		constantBigStorage.ensureStorageAllocated(numConstants);
		
		for (int i = 0; i < numConstants; i++)
		{
			constantBigStorage.add(Constant());
		}

		constantsToUse = constantBigStorage.getRawDataPointer();
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			constants[i] = Constant();
		}

		constantsToUse = constants;
	}

	
}

ApiClass::~ApiClass()
{
	for(auto& flist: functions)
	{
		for(auto& f: flist)
			f = nullptr;
	}
	
	for (int i = 0; i < numConstants; i++)
	{
		constantsToUse[i] = Constant();
	}

	constantsToUse = nullptr;
	constantBigStorage.clear();
}

void ApiClass::addConstant(String constantName, var value)
{
	for (int i = 0; i < numConstants; i++)
	{
		if (constantsToUse[i].id.isNull())
		{
			constantsToUse[i].id = Identifier(constantName);
			constantsToUse[i].value = value;
			return;
		}
	}

	// You're using too many constants...
	jassertfalse;
}

const var ApiClass::getConstantValue(int index) const
{
	if (index >= 0 && index < numConstants)
		return constantsToUse[index].value;

	jassertfalse;
	return var();
}

int ApiClass::getConstantIndex(const Identifier &id) const
{
	for (int i = 0; i < numConstants; i++)
	{
		if (constantsToUse[i].id == id) return i;
	}

	return -1;
}

Identifier ApiClass::getConstantName(int index) const
{
	if (index < numConstants)
		return constantsToUse[index].id;

	jassertfalse;
    return {};
}

void ApiClass::addFunction(const Identifier &id, call0 newFunction)
{
	addFunctionT<0>(id, reinterpret_cast<void*>(newFunction));
}

void ApiClass::addFunction1(const Identifier &id, call1 newFunction)
{
	addFunctionT<1>(id, reinterpret_cast<void*>(newFunction));
}

void ApiClass::addFunction2(const Identifier &id, call2 newFunction)
{
	addFunctionT<2>(id, reinterpret_cast<void*>(newFunction));
}

void ApiClass::addFunction3(const Identifier &id, call3 newFunction)
{
	addFunctionT<3>(id, reinterpret_cast<void*>(newFunction));
}

void ApiClass::addFunction4(const Identifier &id, call4 newFunction)
{
	addFunctionT<4>(id, reinterpret_cast<void*>(newFunction));
}

void ApiClass::addFunction5(const Identifier &id, call5 newFunction)
{
	addFunctionT<5>(id, reinterpret_cast<void*>(newFunction));
}

bool ApiClass::getIndexAndNumArgsForFunction(const Identifier &id, int &index, int &numArgs) const
{
	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		int thisIndex = 0;

		for(auto& idList: ids)
		{
			if(idList[i] == id)
			{
				index = i;
				numArgs = thisIndex;
				return true;
			}

			thisIndex++;
		}
	}

	index = -1;
	numArgs = -1;
	return false;
}

var ApiClass::callFunction(int index, var *args, int numArgs)
{
	if (index > NUM_API_FUNCTION_SLOTS)
	{
		return var();
	}

	switch (numArgs)
	{
	case 0: { auto f = reinterpret_cast<call0>(functions[numArgs][index]); return f(this); }
	case 1: { auto f = reinterpret_cast<call1>(functions[numArgs][index]); return f(this, args[0]); }
	case 2: { auto f = reinterpret_cast<call2>(functions[numArgs][index]); return f(this, args[0], args[1]); }
	case 3: { auto f = reinterpret_cast<call3>(functions[numArgs][index]); return f(this, args[0], args[1], args[2]); }
	case 4: { auto f = reinterpret_cast<call4>(functions[numArgs][index]); return f(this, args[0], args[1], args[2], args[3]); }
	case 5: { auto f = reinterpret_cast<call5>(functions[numArgs][index]); return f(this, args[0], args[1], args[2], args[3], args[4]); }
	}

	return var();
}

void ApiClass::getAllFunctionNames(Array<Identifier> &listToFill) const
{
	listToFill.ensureStorageAllocated(NUM_API_FUNCTION_SLOTS * NumMaxArguments);

	for (int i = 0; i < NUM_API_FUNCTION_SLOTS; i++)
	{
		for(auto& idList: ids)
		{
			if(!idList[i].isNull())
				listToFill.add(idList[i]);
		}
	}

	IdentifierComparator idComp;

	listToFill.sort(idComp);
}

void ApiClass::getAllConstants(Array<Identifier> &ids) const
{
	for (int i = 0; i < numConstants; i++)
	{
		if (!constantsToUse[i].id.isNull()) ids.add(constantsToUse[i].id);
	}
}

ApiClass::Constant::Constant(const Identifier &id_, var value_) :
id(id_),
value(value_)
{

}

ApiClass::Constant::Constant() :
id(Identifier()),
value(var())
{

}

ApiClass::Constant& ApiClass::Constant::operator=(const Constant& other)
{
	id = other.id;
	value = other.value;
	return *this;
}

#if USE_BACKEND
juce::String ApiClass::DiagnosticResult::Item::toConsoleString(Processor* p) const
{
	return HiseJavascriptEngine::toConsoleString(*this, p);
}

String ApiClass::DiagnosticResult::Item::toEditorString() const
{
	String s;

	if (ec != ErrorCode::Unknown)
	{
		auto sev = (Severity)((int)ec / 1000);
		s << "[";
		s << getSeverityString(sev).toUpperCase()[0];
		s << String((int)ec);
		s << "] ";
	}

	s << message;

	if (!suggestions.isEmpty())
		s << ". Use: " << suggestions.joinIntoString(", ");

	return s;
}

#endif

void hise::DiagnosticBase::ShadowParseData::Item::resolveDependencies(DependencyResolver* r)
{
	for (auto d : r->resolveDependencies(this))
	{
		if (d != this)
			dependencies.addIfNotAlreadyThere(d);
	}
}

} // namespace hise
