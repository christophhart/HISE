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
		auto numThisTime = jmin<int>(8192, numToWrite - i);

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

	if(obj.hasProperty(mpid::Filename))
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
	stopThread(1000);

	tempFiles.clear();
}

void State::run()
{
	for(int i = 0; i < jobs.size(); i++)
	{
		currentJob = jobs[i];
		
		auto ok = jobs[i]->runJob();

		currentJob = nullptr;
		
		if(ok.failed())
		{
			navigateOnFinish = false;
			break;
		}
            
		totalProgress = (double)i / (double)jobs.size();
	}
        
	jobs.clear();
        
	MessageManager::callAsync(BIND_MEMBER_FUNCTION_0(State::onFinish));
}

void State::reset(const var& obj)
{
	eventLogger.sendMessage(sendNotificationSync, MessageType::Clear, "");

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
	currentDialog = nullptr;
	currentError = Result::ok();
	currentJob = nullptr;
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
{ return progress; }

void State::Job::setMessage(const String& newMessage)
{
	message = newMessage;

	parent.eventLogger.sendMessage(sendNotificationAsync, MessageType::ProgressMessage, newMessage);

	if(parent.currentDialog != nullptr)
	{
		SafeAsyncCall::repaint(parent.currentDialog.get());
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
	if(currentDialog.get() != nullptr)
	{
		currentDialog->nextButton.setEnabled(currentDialog->currentErrorElement == nullptr);
		currentDialog->prevButton.setEnabled(true);

		auto p = currentDialog->currentPage.get();
		
		if(navigateOnFinish)
		{
			currentDialog->navigate(true);
			navigateOnFinish = false;
		}
	}
}

Result State::Job::runJob()
{
	try
	{
		auto ok = run();
            
		if(auto p = parent.currentDialog.get())
		{
			SafeAsyncCall::repaint(p);
		}

		return ok;
	}
	catch(Result& r)
	{
		if(auto p = parent.currentDialog)
		{
			p->logMessage(MessageType::ProgressMessage, "ERROR: " + r.getErrorMessage());

			MessageManager::callAsync([p]()
			{
				p->repaint();
			});
		}

		return r;
	}
}

void State::addJob(Job::Ptr b, bool addFirst)
{
	if(addFirst)
		jobs.insert(0, b);
	else
		jobs.add(b);
        
	if(!isThreadRunning())
	{
		if(currentDialog != nullptr)
		{
			currentDialog->setCurrentErrorPage(nullptr);
			currentDialog->repaint();
			currentDialog->nextButton.setEnabled(false);
			currentDialog->prevButton.setEnabled(false);
		}
            
		startThread(6);
	}
}

void State::addFileToLog(const std::pair<File, bool>& fileOp)
{
	fileOperations.add(fileOp);
}

void State::bindCallback(const String& functionName, const var::NativeFunction& f)
{
	jsLambdas[functionName] = f;
}

bool State::callNativeFunction(const String& functionName, const var::NativeFunctionArgs& args, var* returnValue)
{
	if(jsLambdas.find(functionName) != jsLambdas.end())
	{
		auto rv = jsLambdas[functionName](args);

		if(returnValue != nullptr)
			*returnValue = rv;

		return true;
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
	case Type::RemoveProperty: parent.getDynamicObject()->removeProperty(key); true;
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
		addAndMakeVisible(dialog = createDialog(state));

		postInit();

		dialog->setFinishCallback(closeFunction);
		dialog->setEnableEditMode(false);
		
		dialog->showFirstPage();
	}

	dialog->setBounds(getLocalBounds());
}
}
}
