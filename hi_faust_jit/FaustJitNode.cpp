#define FAUSTFLOAT float

#include "FaustWrapper.h"
#include "FaustMenuBar.h"

namespace scriptnode {
namespace faust {

// faust_jit_node::faust_jit_node(DspNetwork* n, ValueTree v) :
//      NodeBase(n, v, 0) { }
faust_jit_node::faust_jit_node(DspNetwork* n, ValueTree v) :
	WrapperNode(n, v),
	classId(PropertyIds::ClassId, ""),
	faust(new faust_jit_wrapper())
{
	extraComponentFunction = [](void* o, PooledUIUpdater* u)
	{
		return new FaustMenuBar(static_cast<faust_jit_node*>(o));
	};

	parameterListener.setCallback(getParameterTree(),
								  valuetree::AsyncMode::Synchronously,
								  BIND_MEMBER_FUNCTION_2(faust_jit_node::parameterUpdated));

	File f = getFaustRootFile();
	// Create directory if it's not already there
	if (!f.isDirectory()) {
		auto res = f.createDirectory();
	}
	// we can't init yet, because we don't know the sample rate
	initialise(this);
	setClass(classId.getValue());
}

void faust_jit_node::setupParameters()
{
	// setup parameters from faust code
	for (auto p : faust->ui.parameters)
	{
		DBG("adding parameter " << p->label << ", step: " << p->step);
		auto pd = p->toParameterData();
		addNewParameter(pd);
	}
	DBG("Num parameters in NodeBase: " << getNumParameters());
}

static void updateFaustZone(void* obj, double value)
{
	*static_cast<float*>(obj) = (float)value;
}

// ParameterTree listener callback: This function is called when the ParameterTree changes
void faust_jit_node::parameterUpdated(ValueTree child, bool wasAdded)
{
	if (wasAdded)
	{
		String parameterLabel = child.getProperty(PropertyIds::ID);
		auto faustParameter = faust->ui.getParameterByLabel(parameterLabel).value_or(nullptr);

		// proceed only if this parameter was defined in faust (check by label)
		if (!faustParameter)
			return;

		float* zonePointer = faustParameter->zone;

		// setup dynamic parameter
		auto dp = new scriptnode::parameter::dynamic();
		// install callback and zone pointer into dynamic parameter
		dp->referTo(zonePointer, updateFaustZone);

		// create dynamic_base from dynamic parameter to attach to the new Parameter
		auto dyn_base = new scriptnode::parameter::dynamic_base(*dp);

		// create and setup parameter
		auto p = new scriptnode::Parameter(this, child);
		p->setDynamicParameter(dyn_base);

		NodeBase::addParameter(p);
		{
			auto index = child.getParent().indexOf(child);
			DBG("parameter added, index=" << index);
		}
	}
	else
	{
		String parameterLabel = child.getProperty(PropertyIds::ID);
		DBG("removing parameter: " + parameterLabel);
		NodeBase::removeParameter(parameterLabel);
	}
}

void faust_jit_node::initialise(NodeBase* n)
{
	classId.initialise(n);
	classId.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(faust_jit_node::updateClassId), true);
}


void faust_jit_node::prepare(PrepareSpecs specs)
{
	try
	{
		getRootNetwork()->getExceptionHandler().removeError(this, Error::ErrorCode::IllegalFaustChannelCount);

		lastSpecs = specs;
		faust->prepare(specs);
	}
	catch (scriptnode::Error& e)
	{
		getRootNetwork()->getExceptionHandler().addError(this, e);
	}
}

void faust_jit_node::reset()
{
	faust->reset();
}

void faust_jit_node::process(ProcessDataDyn& data)
{
	if (isBypassed()) return;
	faust->process(data);
}

void faust_jit_node::processFrame(FrameType& data)
{
	if (isBypassed()) return;
	faust->faust_jit_wrapper::processFrame<FrameType>(data);
}

File faust_jit_node::getFaustRootFile()
{
	auto mc = this->getScriptProcessor()->getMainController_();
	auto dspRoot = mc->getCurrentFileHandler().getSubDirectory(FileHandlerBase::DspNetworks);
	return dspRoot.getChildFile("CodeLibrary/" + getStaticId().toString());
}

/*
 * Lookup a Faust source code file for this node.
 * The `basename` is the name without any extension.
 */
File faust_jit_node::getFaustFile(String basename)
{
	auto nodeRoot = getFaustRootFile();
	return nodeRoot.getChildFile(basename + ".dsp");
}

std::vector<std::string> faust_jit_node::getFaustLibraryPaths()
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


void faust_jit_node::addNewParameter(parameter::data p)
{
	if (auto existing = getParameterFromName(p.info.getId()))
		return;

	auto newTree = p.createValueTree();
	getParameterTree().addChild(newTree, -1, getUndoManager());
}

// Remove all HISE Parameters. Parameters will still be present in faust->ui
// and will be cleared in faust->setup() automatically
void faust_jit_node::resetParameters()
{
	DBG("Resetting parameters");
	getParameterTree().removeAllChildren(getUndoManager());
}

String faust_jit_node::getClassId()
{
	return classId.getValue();
}

bool faust_jit_node::removeClassId(String classIdToRemove)
{
	// TODO remove from ValueTree
	if (getClassId() == classIdToRemove)
		return false;
	return true;
}

void faust_jit_node::loadSource()
{
	auto newClassId = getClassId();
	if (faust->getClassId() != newClassId)
		reinitFaustWrapper();
}

void faust_jit_node::createSourceAndSetClass(const String newClassId)
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
void faust_jit_node::logError(String errorMessage)
{
	auto p = dynamic_cast<Processor*>(getScriptProcessor());
	debugError(p, errorMessage);
}

void faust_jit_node::reinitFaustWrapper()
{
	String newClassId = getClassId();
	resetParameters();
	File sourceFile = getFaustFile(newClassId);

	// Do nothing if file doesn't exist:
	if (!sourceFile.existsAsFile()) {
		DBG("Could not load Faust source file (" + sourceFile.getFullPathName() + "): File not found.");
		return;
	}

	DBG("Faust DSP file to load:" << sourceFile.getFullPathName());

	// Load file and recompile
	String code = sourceFile.loadFileAsString();
	bool classIdValid = faust->setCode(newClassId, code.toStdString());
	if (!classIdValid)
	{
		logError("Invalid name for exported C++ class: " + newClassId.toStdString());
		return;
	}

	try
	{
		getRootNetwork()->getExceptionHandler().removeError(this);

		std::string error_msg;
		bool success = faust->setup(getFaustLibraryPaths(), error_msg);
		DBG("Faust initialization: " + std::string(success ? "success" : "failed"));
		if (!success)
		{
			logError(error_msg);
		}
		setupParameters();
	}
	catch (scriptnode::Error& e)
	{
		getRootNetwork()->getExceptionHandler().addError(this, e);
	}

	// Setup DSP
	
}

void faust_jit_node::setClass(const String& newClassId)
{
	classId.storeValue(newClassId, getUndoManager());
	updateClassId({}, newClassId);
	loadSource();
}

void faust_jit_node::updateClassId(Identifier, var newValue)
{
	auto newId = newValue.toString();

	DBG(newId);
	if (newId.isNotEmpty())
	{
		auto nb = getRootNetwork()->codeManager.getOrCreate(getTypeId(), Identifier(newValue.toString()));
		// TODO: workbench
	}
}

StringArray faust_jit_node::getAvailableClassIds()
{
	return getRootNetwork()->codeManager.getClassList(getTypeId());
}


} // namespace faust
} // namespace scriptnode
