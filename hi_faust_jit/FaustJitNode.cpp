#define FAUSTFLOAT float

#include "FaustWrapper.h"
#include "FaustMenuBar.h"

namespace scriptnode {
namespace faust {

// faust_jit_node::faust_jit_node(DspNetwork* n, ValueTree v) :
//      NodeBase(n, v, 0) { }
faust_jit_node_base::faust_jit_node_base(DspNetwork* n, ValueTree v) :
	WrapperNode(n, v),
	classId(PropertyIds::ClassId, ""),
    compileResult(Result::ok())
{
	extraComponentFunction = [](void* o, PooledUIUpdater* u)
	{
		return new FaustMenuBar(static_cast<faust_jit_node_base*>(o));
	};

	parameterListener.setCallback(getParameterTree(),
								  valuetree::AsyncMode::Synchronously,
								  BIND_MEMBER_FUNCTION_2(faust_jit_node_base::parameterUpdated));

    n->faustManager.addFaustListener(this);
    
	File f = getFaustRootFile();
	// Create directory if it's not already there
	if (!f.isDirectory()) {
		auto res = f.createDirectory();
	}
	
}

faust_jit_node_base::~faust_jit_node_base()
{
    getRootNetwork()->faustManager.removeFaustListener(this);
}

void faust_jit_node_base::setupParameters()
{
    // resize the parameter::dynamic_list amount
    if(getLastResult().wasOk())
        setNodeProperty(PropertyIds::NumParameters, getNumFaustModulationOutputs());
    
	auto pTree = getParameterTree();

	for (int i = 0; i < pTree.getNumChildren(); i++)
	{
		auto c = pTree.getChild(i);
		auto id = c[PropertyIds::ID].toString();
		
		if (!hasFaustParameter(id))
		{
			pTree.removeChild(c, getUndoManager());
			i--;
		}
	}

    auto numHolders = parameterHolders.size();
	auto numParameters = getNumFaustParameters();
    
    // make sure there are enough parameter holders
    while(numHolders < numParameters)
    {
        auto h = new parameter::dynamic_base_holder();
        h->setAllowForwardToParameter(false);
        parameterHolders.add(h);
        numHolders = parameterHolders.size();
    }
    
	// setup parameters from faust code

	setupFaustParameters();

	DBG("Num parameters in NodeBase: " << getNumParameters());
}

// ParameterTree listener callback: This function is called when the ParameterTree changes
void faust_jit_node_base::parameterUpdated(ValueTree child, bool wasAdded)
{
	if (wasAdded)
	{
        auto index = child.getParent().indexOf(child);
        
        while(parameterHolders.size() < (index + 1))
        {
            auto h = new parameter::dynamic_base_holder();
            h->setAllowForwardToParameter(false);
            parameterHolders.add(h);
        }
        
        jassert(isPositiveAndBelow(index, parameterHolders.size()));
        
        auto p = new scriptnode::Parameter(this, child);
        p->setDynamicParameter(parameterHolders[index]);

        NodeBase::addParameter(p);
        
        
#if 0
        
        
		String parameterLabel = child.getProperty(PropertyIds::ID);
		auto faustParameter = faust->ui.getParameterByLabel(parameterLabel).value_or(nullptr);

		// proceed only if this parameter was defined in faust (check by label)
		if (!faustParameter)
			return;

		float* zonePointer = faustParameter->zone;

#if 0
        faustParameter.toParameterData().callback
        
		// setup dynamic parameter
		auto dp = new scriptnode::parameter::dynamic();
		// install callback and zone pointer into dynamic parameter
		dp->referTo(faustParameter, faust_ui::Parameter::setParameter);
#endif

        auto pData = faustParameter->toParameterData();
        
        
        
		// create dynamic_base from dynamic parameter to attach to the new Parameter
        auto dyn_base = new scriptnode::parameter::dynamic_base(pData.callback);

		// create and setup parameter
		auto p = new scriptnode::Parameter(this, child);
		p->setDynamicParameter(dyn_base);

		NodeBase::addParameter(p);
		{
			auto index = child.getParent().indexOf(child);
			DBG("parameter added, index=" << index);
		}
#endif
	}
	else
	{
		String parameterLabel = child.getProperty(PropertyIds::ID);
		DBG("removing parameter: " + parameterLabel);
		NodeBase::removeParameter(parameterLabel);
	}
}

void faust_jit_node_base::initialise(NodeBase* n)
{
	classId.initialise(n);
	classId.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(faust_jit_node_base::updateClassId), true);
}

File faust_jit_node_base::getFaustRootFile()
{
	auto mc = this->getScriptProcessor()->getMainController_();
	auto dspRoot = mc->getCurrentFileHandler().getSubDirectory(FileHandlerBase::DspNetworks);
	return dspRoot.getChildFile("CodeLibrary/" + getStaticId().toString());
}

/*
 * Lookup a Faust source code file for this node.
 * The `basename` is the name without any extension.
 */
File faust_jit_node_base::getFaustFile(String basename)
{
	auto nodeRoot = getFaustRootFile();
	return nodeRoot.getChildFile(basename + ".dsp");
}

std::vector<std::string> faust_jit_node_base::getFaustLibraryPaths()
{
	std::vector<std::string> paths;
	paths.push_back(getFaustRootFile().getFullPathName().toStdString());
	const auto& data = dynamic_cast<GlobalSettingManager*>(this->getScriptProcessor()->getMainController_())->getSettingsObject();

	juce::String faustPath = data.getSetting(hise::HiseSettings::Compiler::FaustPath);
	if (faustPath.length() > 0) {
		DBG("Global Faust Path: " + faustPath);
		auto globalFaustLibraryPath = juce::File(faustPath).getChildFile("share").getChildFile("faust");
		if (globalFaustLibraryPath.isDirectory()) {
			paths.push_back(globalFaustLibraryPath.getFullPathName().toStdString());
		}
	}

	return paths;
}


void faust_jit_node_base::addNewParameter(parameter::data p)
{
	if (auto existing = getParameterFromName(p.info.getId()))
    {
        return;
    }

	auto newTree = p.createValueTree();
	getParameterTree().addChild(newTree, -1, getUndoManager());
}

void faust_jit_node_base::resetParameters()
{
    // clear all parameter holders (most of them will be updated with the new zone
    // but there might be some remaining parameters that could point to a dangling
    // zone when modulated, also the zones are invalidated during compilation
    // so accessing those while compiling will cause a crash too
    for(auto ph: parameterHolders)
    {
        auto h = dynamic_cast<parameter::dynamic_base_holder*>(ph);
        h->setParameter(this, nullptr);
    }
}

String faust_jit_node_base::getClassId()
{
	return classId.getValue();
}

bool faust_jit_node_base::removeClassId(String classIdToRemove)
{
	// TODO remove from ValueTree
	if (getClassId() == classIdToRemove)
		return false;
	return true;
}

void faust_jit_node_base::loadSource()
{
	auto newClassId = getClassId();
	if (getFaustClassId() != newClassId)
    {
        auto sourceFile = getFaustFile(newClassId);
        getRootNetwork()->faustManager.sendCompileMessage(sourceFile, sendNotificationSync);
    }
}

void faust_jit_node_base::createSourceAndSetClass(const String newClassId)
{
	File sourceFile = getFaustFile(newClassId);
	// Create new file if necessary
	if (!sourceFile.existsAsFile()) {
		auto res = sourceFile.create();
		if (res.failed()) {
			std::cerr << "Failed creating file \"" + sourceFile.getFullPathName() + "\"" << std::endl;
			return;
		}
		sourceFile.appendText("// Faust Source File: " + newClassId + "\n"
							  "// Created with HISE on " + juce::Time::getCurrentTime().formatted("%Y-%m-%d") + "\n"
							  "import(\"stdfaust.lib\");\n"
							  "process = _, _;\n");
	}
	setClass(newClassId);
}
void faust_jit_node_base::logError(String errorMessage)
{
	auto p = dynamic_cast<Processor*>(getScriptProcessor());
	debugError(p, errorMessage);
}

void faust_jit_node_base::reinitFaustWrapper()
{
	String newClassId = getClassId();
	//resetParameters();
	File sourceFile = getFaustFile(newClassId);


	String code;

	auto mc = getRootNetwork()->getMainController();
	auto ef = mc->getExternalScriptFile(sourceFile, false);

	if(ef != nullptr)
		code = ef->getFileDocument().getAllContent();
	else if (sourceFile.existsAsFile())
		code = sourceFile.loadFileAsString();
	else
	{
		DBG("Could not load Faust source file (" + sourceFile.getFullPathName() + "): File not found.");
		return;
	}
	
	DBG("Faust DSP file to load:" << sourceFile.getFullPathName());
	
	bool classIdValid = setFaustCode(newClassId, code.toStdString());

	if (!classIdValid)
	{
		logError("Invalid name for exported C++ class: " + newClassId.toStdString());
		return;
	}

	try
	{
        resetParameters();
        
		getRootNetwork()->getExceptionHandler().removeError(this);

		std::string error_msg;
		bool success = setupFaust(getFaustLibraryPaths(), error_msg);
		DBG("Faust initialization: " + std::string(success ? "success" : "failed"));

		if (success)
		{
            compileResult = Result::ok();
			debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), "Faust file " + sourceFile.getFileName() + " compiled OK");
		}
		else
		{
            compileResult = Result::fail(String(error_msg));
            
			getRootNetwork()->getExceptionHandler().addCustomError(this, Error::ErrorCode::CompileFail, String(error_msg));
			logError(error_msg);
		}
        
        if(getNumFaustModulationOutputs() > HISE_NUM_MAX_FAUST_MOD_SOURCES)
        {
            Error e;
            e.error = Error::ErrorCode::TooManyParameters;
            e.actual = getNumFaustModulationOutputs();
            e.expected = HISE_NUM_MAX_FAUST_MOD_SOURCES;
            
            throw e;
        }
        
		setupParameters();
	}
	catch (scriptnode::Error& e)
	{
		getRootNetwork()->getExceptionHandler().addError(this, e);
	}

    //auto p = dynamic_cast<Processor*>(getScriptProcessor());
    
    // The faust menu bar might be resized if a mod 
    setCachedSize(-1, -1);
    
	// Setup DSP
	
}

Result faust_jit_node_base::compileFaustCode(const File& f)
{
    String newClassId = getClassId();
    File sourceFile = getFaustFile(newClassId);
    
    if(sourceFile == f)
    {
        reinitFaustWrapper();
    
        return compileResult;
    }
    
    return Result::ok();
}

void faust_jit_node_base::setClass(const String& newClassId)
{
	// If the faust file is changed, remove all parameters from the previous patch
	if (getClassId() != newClassId)
		getParameterTree().removeAllChildren(getUndoManager());

	setNodeProperty(PropertyIds::ClassId, newClassId);
}

void faust_jit_node_base::updateClassId(Identifier, var newValue)
{
	auto newId = newValue.toString();

	if (newId.isNotEmpty())
	{
		loadSource();
	}
}

StringArray faust_jit_node_base::getAvailableClassIds()
{
	return getRootNetwork()->codeManager.getClassList(getTypeId(), "*.dsp");
}


} // namespace faust
} // namespace scriptnode
