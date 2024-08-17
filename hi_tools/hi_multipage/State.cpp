/*  ===========================================================================
 *
 *   This file is part of HISE.
 *   Copyright 2016 Christoph Hart
 *
 *   HISE is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   HISE is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Commercial licenses for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licensing:
 *
 *   http://www.hise.audio/
 *
 *   HISE is based on the JUCE library,
 *   which must be separately licensed for closed source applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */




namespace hise
{
using namespace juce;

namespace multipage {
using namespace juce;

	
#if HISE_MULTIPAGE_INCLUDE_EDIT
struct State::StateProvider: public ApiProviderBase
{
	static DebugInformationBase::Ptr createRecursive(const String& name, const var& value);

	struct SettableObject: public SettableDebugInfo
	{
		virtual Ptr getChildElement(int index)
        {
			auto id = obj.getDynamicObject()->getProperties().getName(index);

			String s;
            s << name << '.' << id;

            return createRecursive(s, obj[id]);
        }

        int getNumChildElements() const override
        {
	        return obj.getDynamicObject()->getProperties().size();
        }

		var obj;
	};

    struct SettableArray: public SettableDebugInfo
    {
        virtual Ptr getChildElement(int index)
        {
            String s;
            s << name << '[' << String(index) << ']';
            return createRecursive(s, list[index]);
        }

        int getNumChildElements() const override
        {
	        return list.size();
        }

        var list;
    };

    StateProvider(State& parent_):
      parent(parent_)
    {}

        /** Override this method and return the number of all objects that you want to use. */
    int getNumDebugObjects() const override
    {
	    return parent.globalState.getDynamicObject()->getProperties().size();
    }

	DebugInformationBase::Ptr getDebugInformation(int index) override
    {
	    auto id = parent.globalState.getDynamicObject()->getProperties().getName(index);
        auto value = parent.globalState[id];
        return createRecursive(id.toString(), value);
    }

    State& parent;
};

DebugInformationBase::Ptr State::StateProvider::createRecursive(const String& name, const var& value)
{
	SettableDebugInfo* ni;

	if(value.isArray())
	{
		auto sa = new SettableArray();
		sa->list = value;
		ni = sa;
	}
	else if (auto no = value.getDynamicObject())
	{
		auto so = new SettableObject();
		so->obj = no;
		ni = so;
	}
	else
	{
		ni = new SettableDebugInfo();
	}

	ni->name = name;
	ni->autocompleteable = false;
	ni->value = value.toString();

	return ni;
}
#endif

String Asset::getTypeString(Type t)
{
	switch(t)
	{
	case Type::Image: return "Image";
	case Type::File: return "File";
	case Type::Font: return "Font";
	case Type::Text: return "Text";
	case Type::Stylesheet: return "CSS";
	case Type::Archive: return "Archive";
	case Type::numTypes: break;
	default: ;
	}

	return {};
}

Asset::Type Asset::getType(const File& f)
{
	auto extension = f.getFileExtension();

	if(auto format = ImageFileFormat::findImageFormatForFileExtension(f))
		return Type::Image;
	else if(extension == ".txt" || extension == ".md" || extension == ".js" || extension == ".html")
		return Type::Text;
	else if(extension == ".ttf" || extension == ".otf")
		return Type::Font;
	else if(extension == ".css")
		return Type::Stylesheet;
	else if(extension == ".zip")
		return Type::Archive;
	else
		return Type::File;
}

String Asset::toText(bool reload)
{
	if(type == Type::Text || type == Type::Stylesheet)
	{
		if(reload && File::isAbsolutePath(filename))
		{
			loadFromFile();
		}

		return data.toString();
	}
	else
		return {};
}

void Asset::writeCppLiteral(OutputStream& c, const String& nl, ReferenceCountedObject* job_) const
{
	auto& job = *static_cast<State::Job*>(job_);

	job.getProgress() = 0.0;
	job.setMessage("Compressing " + id);

	zstd::ZDefaultCompressor comp;

	MemoryBlock compressed;

	int numBytes;

	if(matchesOS())
	{
		comp.compress(data, compressed);

		job.setMessage("Embedding " + id);

		numBytes = static_cast<int>(compressed.getSize());
	}
	else
	{
		c << nl << "// do not include for current OS...";

		char x1[1] = { 0 };
		compressed.append(x1, 1);
		numBytes = 1;
	}
	
	c << nl << "static const unsigned char " << id << "[" << String(numBytes) << "] = { ";
	
	auto bytePtr = static_cast<const uint8*>(compressed.getData());

	for(int i = 0; i < numBytes; i++)
	{
		c << String((int)bytePtr[i]);
            
		if(i < (numBytes-1))
			c << ",";

		if ((i % 40) == 39)
		{
			job.getProgress() = 0.5 + 0.5 * ((double)i / (double)numBytes);
			c << nl;
		}
	}

	c << " };";

	c << nl << "static constexpr char " << id << "_Filename[" << String(filename.length()+1) << "] = ";
	c << filename.replaceCharacter('\\', '/').quoted() << ";";

	c << nl << "static constexpr Asset::Type " << id << "_Type = Asset::Type::" << getTypeString(type) << ";";
}

var Asset::toJSON(bool embedData, const File& currentRoot) const
{
	auto v = new DynamicObject();

	v->setProperty(mpid::Type, (int)type);
	v->setProperty(mpid::ID, id);
	v->setProperty(mpid::RelativePath, useRelativePath);
	v->setProperty(mpid::OperatingSystem, (int)os);

	if(embedData)
	{
		MemoryBlock compressed;
		zstd::ZDefaultCompressor comp;
		comp.compress(data, compressed);
		v->setProperty(mpid::Data, var(compressed));
	}
	else
		v->setProperty(mpid::Filename, getFilePath(currentRoot));
	return var(v);
}

String Asset::getFilePath(const File& currentRoot) const
{
	if(useRelativePath)
		return File(filename).getRelativePathFrom(currentRoot).replaceCharacter('\\', '/');
	else
		return filename;
}

bool Asset::writeToFile(const File& targetFile, ReferenceCountedObject* job_) const
{
	if(!matchesOS())
		throw Result::fail("Trying to access an asset that isn't included in the current OS");

	auto& job = *static_cast<State::Job*>(job_);

	MemoryInputStream mis(data, false);
	targetFile.deleteFile();
	FileOutputStream fos(targetFile);

	if(fos.failedToOpen())
		throw Result::fail("Error at writing file: " + fos.getStatus().getErrorMessage());

	auto numToWrite = mis.getTotalLength();

	for(int i = 0; i < numToWrite; i += 8192)
	{
		auto numThisTime = jmin<int>(8192, (int)numToWrite - i);

		auto ok = fos.writeFromInputStream(mis, numThisTime) == numThisTime;

		if(!ok || job.getThread().threadShouldExit())
			throw Result::fail("File write operation failed at " + String(i/1024) + "kb. Disk full?");

		job.getProgress() = (double)i / (double)numToWrite;
	}

	auto ok = mis.getPosition() == mis.getTotalLength();

	if(ok)
	{
		fos.flush();
	}

	return ok;
}

String Asset::getOSName(TargetOS os)
{
	switch(os)
	{
	case TargetOS::All: return "All";
	case TargetOS::Windows: return "Win";
	case TargetOS::macOS: return "Mac";
	case TargetOS::Linux: return "Linux";
	case TargetOS::numTargetOS:;
	default: return "All";
	}
}

Asset::Ptr Asset::fromVar(const var& obj, const File& currentRoot)
{
	auto t = (Type)(int)obj[mpid::Type];
	auto id = obj[mpid::ID].toString();

	if(obj.hasProperty(mpid::Filename) && currentRoot.isDirectory())
	{
		auto filePath = obj[mpid::Filename].toString();

		File f;

		if(obj[mpid::RelativePath])
		{
			f = currentRoot.getChildFile(filePath);
		}
		else
			f = File(filePath);

		auto a = fromFile(f);
		jassert(a->type == t);
		a->id = id;
		a->useRelativePath = obj[mpid::RelativePath];
		a->os = (TargetOS)(int)obj[mpid::OperatingSystem];
		return a;
	}
	else
	{
		return fromMemory(std::move(*obj[mpid::Data].getBinaryData()), t, obj[mpid::Filename].toString(), id);
	}
}

void Asset::loadFromFile()
{
	MemoryOutputStream mos;
	File f(filename);
	FileInputStream fis(f);
        
	if(fis.openedOk())
	{
		mos.writeFromInputStream(fis, fis.getTotalLength());
		data = mos.getMemoryBlock();
	}
}

State::State(const var& obj, const File& currentRootDirectory_):
	Thread("Tasks"),
	currentRootDirectory(currentRootDirectory_),
#if HISE_MULTIPAGE_INCLUDE_EDIT
	stateProvider(new StateProvider(*this)),
#endif
	currentError(Result::ok())
{
	eventLogger.setEnableQueue(true);

	reset(obj);
}

State::~State()
{
	onDestroy();

	
}

void State::run()
{
	
	for(int i = 0; i < jobs.size(); i++)
	{
		currentJob = jobs[i];
		auto ok = Result::ok();


		if(!completedJobs.contains(currentJob))
		{
			ok = jobs[i]->runJob();

			if(threadShouldExit())
				return;

			
		}

		if(ok.wasOk())
			completedJobs.addIfNotAlreadyThere(currentJob);

		currentJob = nullptr;
	
		if(ok.failed())
		{
			navigateOnFinish = false;
			break;
		}

		totalProgress = (double)i / (double)jobs.size();
	}
    
	jobs.clear();

	SafeAsyncCall::call<State>(*this, [](State& s){ s.onFinish(); });
}

void State::reset(const var& obj)
{
	stopThread(1000);


	eventLogger.sendMessage(sendNotificationSync, MessageType::Clear, "");

	onDestroy();

	if(auto gs = obj[mpid::GlobalState].getDynamicObject())
		globalState = var(gs->clone().get());
	else
		globalState = var(new DynamicObject());

	assets.clear();

	if(auto assetList = obj[mpid::Assets].getArray())
	{
		for(const auto& av: *assetList)
		{
			assets.add(Asset::fromVar(av, currentRootDirectory));
		}
	}
	
	currentPageIndex = 0;

	

	
}

ApiProviderBase* State::getProviderBase()
{
	return stateProvider.get();
}

Font State::loadFont(String fontName) const
{
	jassert(fontName != "default");

	if(fontName.startsWith("${"))
	{
		auto id = fontName.substring(2, fontName.length() - 1);

		for(auto a: assets)
		{
			if(a->id == id)
			{
				return a->toFont();
			}
		}
	}


	return Font(fontName, 13.0f, Font::plain);
}

void State::onDestroy()
{
	stopThread(1000);
	currentJob = nullptr;
	jobs.clear();
	completedJobs.clear();
	
	var v[2] = { var(false), globalState };
	var::NativeFunctionArgs args(var(), v, 2);
	callNativeFunction("onFinish", args, nullptr);

	for(auto d: currentDialogs)
	{
		if(d != nullptr)
			d->onStateDestroy();
	}

	jsLambdas.clear();
	currentDialogs.clear();
	tempFiles.clear();

	currentError = Result::ok();
	
}

void State::addEventListener(const String& eventType, const var& functionObject)
{
	addCurrentEventGroup();
        
	eventListeners[currentEventGroup].addIfNotAlreadyThere({eventType, functionObject});
}

void State::removeEventListener(const String& eventType, const var& functionObject)
{
	addCurrentEventGroup();

	for(auto& map: eventListeners)
	{
		map.second.removeAllInstancesOf({eventType, functionObject});
	}
}

void State::addCurrentEventGroup()
{
	if(eventListeners.find(currentEventGroup) == eventListeners.end())
	{
		eventListeners[currentEventGroup] = {};
	}
}

void State::clearAndSetGroup(const String& groupId)
{
	currentEventGroup = groupId;
	addCurrentEventGroup();
	eventListeners[currentEventGroup] = {};
}

void State::callEventListeners(const String& eventType, const Array<var>& args)
{
	auto ok = Result::ok();

	addCurrentEventGroup();
        
	auto engine = createJavascriptEngine();

	for(auto& map: eventListeners)
	{
		for(auto& v: map.second)
		{
			if(v.first == eventType)
			{
				auto to = new DynamicObject();
                    
				engine->callFunctionObject(to, v.second, var::NativeFunctionArgs(var(to), args.getRawDataPointer(), args.size()), &ok);
			}

			if(ok.failed())
				break;
		}
	}

	if(ok.failed())
	{
		jassertfalse;
	}
}


simple_css::StyleSheet::Collection State::getStyleSheet(const String& name, const String& additionalStyle) const
{
	if(name.startsWith("${"))
	{
		auto id = name.substring(2, name.length() - 1);

		for(auto a: assets)
		{
			if(a->id == id)
				return a->toStyleSheet(additionalStyle);
		}
	}

	auto list = StringArray::fromLines(DefaultCSSFactory::getTemplateList());

	auto idx = list.indexOf(name);

	if(idx != -1)
		return DefaultCSSFactory::getTemplateCollection((DefaultCSSFactory::Template)idx, additionalStyle);

	return {};
}

JavascriptEngine* State::createJavascriptEngine()
{
	if(javascriptEngine.get() != nullptr)
		return javascriptEngine.get();
	
	eventLogger.sendMessage(getNotificationTypeForCurrentThread(), MessageType::Javascript, "Prepare Javascript execution...");

	javascriptEngine = std::make_unique<JavascriptEngine>();

	
	javascriptEngine->registerNativeObject("Console", new LogFunction(*this));
	javascriptEngine->registerNativeObject("document", new Dom(*this));
	javascriptEngine->registerNativeObject("state", globalState.getDynamicObject());
	
	return javascriptEngine.get();
}

String State::getAssetReferenceList(Asset::Type t) const
{
	String s = "None\n";

	for(auto a: assets)
	{
		if(a->type == t)
		{
			s << a->toReferenceVariable() << "\n";
		}
	}

	return s;
}

String State::loadText(const String& assetVariable, bool forceReload) const
{
	if(assetVariable.isEmpty() || assetVariable == "None")
		return {};

	auto id = assetVariable.substring(2, assetVariable.length() - 1);

	for(auto a: assets)
	{
		if(!(a->type == Asset::Type::Text || a->type == Asset::Type::Stylesheet))
			continue;

		if(a->id == id || a->filename.endsWith(assetVariable))
		{
			return a->toText(forceReload);
		}
	}

	return assetVariable;
}

Image State::loadImage(const String& assetVariable) const
{
	if(assetVariable.isEmpty() || assetVariable == "None")
		return {};

	auto id = assetVariable.substring(2, assetVariable.length() - 1);

	for(auto a: assets)
	{
		if(a->id == id)
		{
			return a->toImage();
		}
	}

	return Image();
}

void State::setLogFile(const File& newLogFile)
{
	if(logFile != File())
		return;

	logFile = newLogFile;

	eventLogger.sendMessage(sendNotificationSync, MessageType::Navigation, "Added file logger " + logFile.getFullPathName());

	if(logFile != File())
	{
		logFile.replaceWithText("Logfile " + Time::getCurrentTime().toISO8601(true) + "\n\n");
        
		this->eventLogger.addListener(*this, [](State& s, MessageType t, const String& message)
		{
			FileOutputStream fos(s.logFile);

			if(fos.openedOk())
			{
				fos << message << "\n";
				fos.flush();
			}
		});
	}
}

State::Job::Job(State& rt, const var& obj):
	parent(rt),
	localObj(obj)
{}

State::Job::~Job()
{}

bool State::Job::matches(const var& obj) const
{
	return localObj[mpid::ID] == obj[mpid::ID];
}

double& State::Job::getProgress()
{ return enableProgress ? progress : unusedProgess; }

void State::Job::setMessage(const String& newMessage)
{
	if(!enableProgress)
		return;

	message = newMessage;

	parent.eventLogger.sendMessage(sendNotificationAsync, MessageType::ProgressMessage, newMessage);

	for(auto d: parent.currentDialogs)
	{
		SafeAsyncCall::repaint(d.get());
	}
}

void State::Job::updateProgressBar(ProgressBar* b) const
{
	if(message.isNotEmpty())
		b->setTextToDisplay(message);
}

State::Job::Ptr State::getJob(const var& obj)
{
	for(auto j: jobs)
	{
		if(j->matches(obj))
			return j;
	}

	for(auto j: completedJobs)
	{
		if(j->matches(obj))
			return j;
	}
        
	return nullptr;
}

var State::getGlobalSubState(const Identifier& id)
{
	if(globalState.hasProperty(id))
		return globalState[id];

	var no = new DynamicObject();
	globalState.getDynamicObject()->setProperty(id, no);
	return no;
}

void State::onFinish()
{
	for(auto d: currentDialogs)
	{
		if(d != nullptr)
		{

			d->nextButton.setEnabled(d->currentErrorElement == nullptr || 
								     d->currentErrorElement->getInfoObject()[mpid::EventTrigger].toString() == "OnCall");
			d->prevButton.setEnabled(true);
			
			if(navigateOnFinish)
				d->navigate(true);
		}
	}

	navigateOnFinish = false;
}

Result State::Job::runJob()
{
	try
	{
		auto ok = run();

		for(auto d: parent.currentDialogs)
			SafeAsyncCall::repaint(d.get());
		
		return ok;
	}
	catch(Result& r)
	{
		parent.logMessage(MessageType::ProgressMessage, "ERROR: " + r.getErrorMessage());

		for(auto d: parent.currentDialogs)
			SafeAsyncCall::repaint(d.get());

		return r;
	}
}

void State::addJob(Job::Ptr b, bool addFirst)
{
	if(completedJobs.contains(b))
		return;

	if(!jobs.contains(b))
	{
		if(addFirst)
			jobs.insert(0, b);
		else
			jobs.add(b);
	}
        
	if(!isThreadRunning())
	{
		for(auto d: currentDialogs)
		{
			d->setCurrentErrorPage(nullptr);
			d->repaint();
			d->nextButton.setEnabled(false);
			d->prevButton.setEnabled(false);
		}
            
		startThread(6);
	}
}

String State::getFileLog() const
{
	String log;
	String nl = "\n";

	for(auto& f: fileOperations)
	{
		log << (f.second ? '+' : '-');
		log << f.first.getFullPathName();
		log << nl;
	}

	return log;
}

void State::addFileToLog(const std::pair<File, bool>& fileOp)
{
	fileOperations.add(fileOp);
}

void State::bindCallback(const String& functionName, const var::NativeFunction& f)
{
	if(!f)
		jsLambdas.erase(functionName);
	else
		jsLambdas[functionName] = f;
}

bool State::callNativeFunction(const String& functionName, const var::NativeFunctionArgs& args, var* returnValue)
{
	if(hasNativeFunction(functionName))
	{
		auto rv = jsLambdas[functionName](args);

		if(returnValue != nullptr)
			*returnValue = rv;

		return true;
	}
	else
	{
		String message;
		message << "Firing custom callback: " << functionName;
		message << " - args: ";

		for(int i = 0; i < args.numArguments; i++)
		{
			message << JSON::toString(args.arguments[i], true);

			if(i != args.numArguments - 1)
				message << ", ";
		}

		logMessage(MessageType::ActionEvent, message);
	}

	return false;
}

var CallableAction::operator()(const var::NativeFunctionArgs& args)
{
	globalState = args.thisObject;

	if(state.currentJob != nullptr)
		return perform(*state.currentJob);
        
	jassertfalse;
	return var();
}

var CallableAction::get(const Identifier& id) const
{
	return globalState[id];
}

LambdaAction::LambdaAction(State& s, const LambdaFunctionWithObject& of_):
	CallableAction(s),
	of(of_)
{}

LambdaAction::LambdaAction(State& s, const LambdaFunction& lf_):
	CallableAction(s),
	lf(lf_)
{}

var LambdaAction::perform(multipage::State::Job& t)
{
	if(lf)
		return lf(t);
	else if (of)
		return of(t, globalState);

	jassertfalse;
	return var();
}

UndoableVarAction::UndoableVarAction(const var& parent_, const Identifier& id, const var& newValue_):
	actionType(newValue_.isVoid() ? Type::RemoveProperty : Type::SetProperty),
	parent(parent_),
	key(id),
	index(-1),
	oldValue(parent[key]),
	newValue(newValue_)
{}

UndoableVarAction::UndoableVarAction(const var& parent_, int index_, const var& newValue_):
	actionType(newValue_.isVoid() ? Type::RemoveChild : Type::AddChild),
	parent(parent_),
	index(index_),
	oldValue(isPositiveAndBelow(index, parent.size()) ? parent[index] : var()),
	newValue(newValue_)
{}

bool UndoableVarAction::perform()
{
	switch(actionType)
	{
	case Type::SetProperty: parent.getDynamicObject()->setProperty(key, newValue); return true;
	case Type::RemoveProperty: parent.getDynamicObject()->removeProperty(key); return true;
	case Type::AddChild: parent.getArray()->insert(index, newValue); return true;
	case Type::RemoveChild: return parent.getArray()->removeAllInstancesOf(oldValue) > 0;
	default: return false;
	}
}

bool UndoableVarAction::undo()
{
	switch(actionType)
	{
	case Type::SetProperty: parent.getDynamicObject()->setProperty(key, oldValue); return true;
	case Type::RemoveProperty: parent.getDynamicObject()->setProperty(key, oldValue); return true;
	case Type::AddChild: parent.getArray()->removeAllInstancesOf(newValue); return true;
	case Type::RemoveChild: parent.getArray()->insert(index, oldValue); return true;
	default: ;
	}

	

	return false;
}

String MonolithData::getMarkerName(Markers m)
{
	switch(m)
	{
	case MonolithBeginVersion: return "Version Number";
	case MonolithBeginJSON: return "MonolithBeginJSON";
	case MonolithEndJSON: return "MonolithEndJSON";
	case MonolithBeginAssets: return "MonolithBeginAssets";
	case MonolithAssetJSONStart: return "MonolithAssetJSONStart";
	case MonolithAssetJSONEnd: return "MonolithAssetJSONEnd";
	case MonolithAssetStart: return "MonolithAssetStart";
	case MonolithAssetEnd: return "MonolithAssetEnd";
	case MonolithEndAssets: return "MonolithEndAssets";
	default: return {};
	}
}

MonolithData::MonolithData(InputStream* input_):
	input(input_)
{}

int64 MonolithData::expectFlag(Markers m, bool throwIfMismatch)
{
	static const Array<Markers> beginMarkers = { MonolithBeginJSON, MonolithAssetJSONStart, MonolithAssetStart };

	auto isBeginMarker = beginMarkers.contains(m);
        
	auto flag = input->readInt();

	if(flag == m)
	{
		return isBeginMarker ? input->readInt64() : 0;
	}
	else if (throwIfMismatch)
	{
		throw String("Expected marker " + getMarkerName(m));
	}
	else
		return 0;
}

var MonolithData::readJSON(int64 numToRead)
{
	MemoryBlock mb;
	input->readIntoMemoryBlock(mb, numToRead);
	String jsonString;
	zstd::ZDefaultCompressor comp;
	comp.expand(mb, jsonString);
	var obj;
	auto r = JSON::parse(jsonString, obj);

	if(!r.wasOk())
		throw r.getErrorMessage();

	return obj;
}

multipage::Dialog* MonolithData::create(State& state, bool allowVersionMismatch)
{
    int64 numToRead = -1;
    
    try
    {
        expectFlag (Markers::MonolithBeginVersion);

        auto thisMajor = input->readInt();
        auto thisMinor = input->readInt();
        auto thisPatch = input->readInt();

        std::array<int, 3> monoVersion = { thisMajor, thisMinor, thisPatch };

        std::array<int, 3> buildVersion = { MULTIPAGE_MAJOR_VERSION, MULTIPAGE_MINOR_VERSION, MULTIPAGE_PATCH_VERSION };

        SemanticVersionChecker svs(monoVersion, buildVersion);

        if(!svs.isExactMatch())
            throw String("Version mismatch. " + svs.getErrorMessage("Payload Build Version", "Installer version"));

        expectFlag (Markers::MonolithEndVersion);
    }
    catch(String& s)
    {
        if(!allowVersionMismatch)
        {
            throw s;
        }
        else
        {
            numToRead = input->readInt64();
        }
    }

    if(numToRead == -1)
	    numToRead = expectFlag(Markers::MonolithBeginJSON);
    
	auto jsonData = readJSON(numToRead);
	expectFlag(Markers::MonolithEndJSON);
	expectFlag(Markers::MonolithBeginAssets);

	state.reset(jsonData);
        
	while(auto metadataSize = expectFlag(Markers::MonolithAssetJSONStart, false))
	{
		auto metadata = readJSON(metadataSize);
		expectFlag(Markers::MonolithAssetJSONEnd);

		auto flag = input->readInt();

		bool isCompressed = true;

		int64 numBytesInData = 0;

		if(flag == Markers::MonolithAssetNoCompressFlag)
		{
			isCompressed = false;
			numBytesInData = expectFlag(Markers::MonolithAssetStart);
		}
		else
		{
			numBytesInData = input->readInt64();
		}
		
		MemoryBlock mb, mb2;
		input->readIntoMemoryBlock(mb, numBytesInData);

		if(isCompressed)
		{
			zstd::ZDefaultCompressor comp;
			comp.expand(mb, mb2);
		}
		else
		{
			std::swap(mb, mb2);
		}
		
		metadata.getDynamicObject()->setProperty(mpid::Data, var(std::move(mb2)));
		auto r = multipage::Asset::fromVar(metadata, state.currentRootDirectory);
		state.assets.add(r);

		expectFlag(Markers::MonolithAssetEnd);
	}

	// caught by the last while loope
	//expectFlag(fis, Markers::MonolithEndAssets);

	if(input->getPosition() != input->getTotalLength())
	{
		throw String("Not EOF");
	}
	
	return new multipage::Dialog(jsonData, state);
}

Result MonolithData::exportMonolith(State& state, OutputStream* target, bool compressAssets, State::Job* job)
{
	// clear the state
	auto json = state.getFirstDialog()->exportAsJSON();
	json.getDynamicObject()->removeProperty(mpid::GlobalState);
	json.getDynamicObject()->removeProperty(mpid::Assets);

	auto c = JSON::toString(json);
	MemoryBlock mb;
	zstd::ZDefaultCompressor comp;
	comp.compress(c, mb);

	if(job != nullptr)
	{
		job->setMessage("Exporting monolith");
	}

	target->writeInt (Markers::MonolithBeginVersion);
	target->writeInt(MULTIPAGE_MAJOR_VERSION);
	target->writeInt(MULTIPAGE_MINOR_VERSION);
	target->writeInt(MULTIPAGE_PATCH_VERSION);
	target->writeInt(Markers::MonolithEndVersion);

	target->writeInt(Markers::MonolithBeginJSON);
	
	target->writeInt64((int64)mb.getSize());
	target->write(mb.getData(), mb.getSize());
	target->writeInt(Markers::MonolithEndJSON);
	target->writeInt(Markers::MonolithBeginAssets);

	for(auto s: state.assets)
	{
		if(!s->matchesOS())
			continue;

		if(job != nullptr)
			job->setMessage("Exporting asset " + s->id);

		auto assetJSON = s->toJSON(false, state.currentRootDirectory);
		assetJSON.getDynamicObject()->removeProperty(mpid::Filename);

		File assetFile(s->filename);

		if(assetFile.existsAsFile())
		{
			assetJSON.getDynamicObject()->setProperty(mpid::Filename, assetFile.getFileName());
		}

		auto metadata = JSON::toString(assetJSON);

		MemoryBlock mb2;
		comp.compress(metadata, mb2);

		target->writeInt(Markers::MonolithAssetJSONStart);
		target->writeInt64(mb2.getSize());
		target->write(mb2.getData(), mb2.getSize());
		target->writeInt(Markers::MonolithAssetJSONEnd);

		if(!compressAssets)
			target->writeInt(Markers::MonolithAssetNoCompressFlag);

		target->writeInt(Markers::MonolithAssetStart);

		bool ok;

		if(compressAssets)
		{
			MemoryBlock mb3;

			comp.compress(s->data, mb3);

			target->writeInt64(mb3.getSize());
			ok = target->write(mb3.getData(), mb3.getSize());
		}
		else
		{
			target->writeInt64(s->data.getSize());
			ok = target->write(s->data.getData(), s->data.getSize());
		}
		
		if(!ok)
			return Result::fail("Error writing asset " + s->id);

		target->writeInt(Markers::MonolithAssetEnd);
	}

	target->writeInt(Markers::MonolithEndAssets);
	target->flush();

	return Result::ok();
}

var MonolithData::getJSON() const
{
	auto flag = input->readInt();

	if(flag == MonolithBeginJSON)
	{
		auto numToRead = input->readInt64();
		MemoryBlock mb;
		auto numRead = input->readIntoMemoryBlock(mb, numToRead);

		if(numRead == numToRead)
		{
			zstd::ZDefaultCompressor comp;
			String jsonString;
			comp.expand(mb, jsonString);

			var obj;
			auto r = JSON::parse(jsonString, obj);

			if(r.wasOk())
				return obj;
			else
				throw String(r.getErrorMessage());
		}
		else
			throw String("Failed to read " + String(numToRead) + " bytes");
	}
}

void HardcodedDialogWithState::setOnCloseFunction(const std::function<void()>& f)
{
	closeFunction = f;

	if(dialog != nullptr)
		dialog->setFinishCallback(closeFunction);
}

void HardcodedDialogWithState::resized()
{
	if(dialog == nullptr)
	{
		if(dialog = createDialog(state))
		{
			addAndMakeVisible(dialog);

			postInit();

			dialog->setFinishCallback(closeFunction);
			dialog->setEnableEditMode(false);
			
			dialog->showFirstPage();
		}
	}

	if(dialog != nullptr)
		dialog->setBounds(getLocalBounds());
}
}
}
