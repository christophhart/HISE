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

String Asset::toText() const
{
	if(type == Type::Text)
		return data.toString();
	else
		return {};
}

void Asset::writeCppLiteral(OutputStream& c, const String& newLine, ReferenceCountedObject* job_) const
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

		numBytes = compressed.getSize();
	}
	else
	{
		c << newLine << "// do not include for current OS...";

		char x1[1] = { 0 };
		compressed.append(x1, 1);
		numBytes = 1;
	}
	
	c << newLine << "static const unsigned char " << id << "[" << String(numBytes) << "] = { ";
	
	auto bytePtr = static_cast<const uint8*>(compressed.getData());

	for(int i = 0; i < numBytes; i++)
	{
		c << String((int)bytePtr[i]);
            
		if(i < (numBytes-1))
			c << ",";

		if ((i % 40) == 39)
		{
			job.getProgress() = 0.5 + 0.5 * ((double)i / (double)numBytes);
			c << newLine;
		}
	}

	c << " };";

	c << newLine << "static constexpr char " << id << "_Filename[" << String(filename.length()+1) << "] = ";
	c << filename.replaceCharacter('\\', '/').quoted() << ";";

	c << newLine << "static constexpr Asset::Type " << id << "_Type = Asset::Type::" << getTypeString(type) << ";";
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
	navigateOnFinish = false;

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



std::unique_ptr<JavascriptEngine> State::createJavascriptEngine(const var& infoObject)
{
	

	struct LogFunction: public ApiObject
	{
		LogFunction(State& s, const var& infoObject_):
		  ApiObject(s),
		  infoObject(infoObject_)
		{
			setMethodWithHelp("print", BIND_MEMBER_FUNCTION_1(LogFunction::print), "Prints a value to the console.");
			setMethodWithHelp("setError", BIND_MEMBER_FUNCTION_1(LogFunction::setError), "Throws an error and displays a popup with the message");
		}

		var infoObject;

		var setError(const var::NativeFunctionArgs& args)
		{
			expectArguments(args, 1);

			if(auto p = state.currentDialog->findPageBaseForInfoObject(infoObject))
			{
				p->setModalHelp(args.arguments[0].toString());
				state.currentDialog->setCurrentErrorPage(p);
			}

			return var();
		}

		var print (const var::NativeFunctionArgs& args)
		{
			expectArguments(args, 1);
			state.eventLogger.sendMessage(getNotificationTypeForCurrentThread(), MessageType::Javascript, args.arguments[0].toString());
			
			return var();
		}
	};

	struct Dom: public ApiObject
	{
		Dom(State& s):
		  ApiObject(s)
		{
			setMethodWithHelp("getElementById", BIND_MEMBER_FUNCTION_1(Dom::getElementById), "Returns an array with all elements that match the given ID");
			setMethodWithHelp("getElementByType", BIND_MEMBER_FUNCTION_1(Dom::getElementByType), "Returns an array with all elements that match the given Type.");
			setMethodWithHelp("updateElement", BIND_MEMBER_FUNCTION_1(Dom::updateElement), "Refreshes the element (call this after you change any property).");
			setMethodWithHelp("getStyleData", BIND_MEMBER_FUNCTION_1(Dom::getStyleData), "Returns the global markdown style data.");
			setMethodWithHelp("setStyleData", BIND_MEMBER_FUNCTION_1(Dom::setStyleData), "Sets the global markdown style data");
		}

		var getElementByType(const var::NativeFunctionArgs& args)
		{
			expectArguments(args, 1);
			Array<var> matches;

			if(state.currentDialog != nullptr)
			{
				auto id = args.arguments[0].toString();

				Component::callRecursive<Dialog::PageBase>(state.currentDialog.get(), [&](Dialog::PageBase* pb)
				{
					if(pb->getPropertyFromInfoObject(mpid::Type) == id)
						matches.add(pb->getInfoObject());

					return false;
				});
			}

			return var(matches);
		}

		var updateElement(const var::NativeFunctionArgs& args)
		{
			expectArguments(args, 1);

			if(state.currentDialog != nullptr)
			{
				auto id = Identifier(args.arguments[0].toString());
				auto dialog = state.currentDialog.get();

				auto f = [id, dialog]()
				{
					Component::callRecursive<Dialog::PageBase>(dialog, [&](Dialog::PageBase* pb)
					{
						if(pb->getId() == id)
						{
							pb->postInit();
							factory::Container::recalculateParentSize(pb);
							dialog->refreshBroadcaster.sendMessage(sendNotificationAsync, dialog->getState().currentPageIndex);
						}
							
						return false;
					});
				};

				if(getNotificationTypeForCurrentThread() == sendNotificationAsync)
					MessageManager::callAsync(f);
				else
					f();
			}

			return var();
		}

		var getElementById(const var::NativeFunctionArgs& args)
		{
			expectArguments(args, 1);

			Array<var> matches;

			if(state.currentDialog != nullptr)
			{
				auto id = Identifier(args.arguments[0].toString());

				Component::callRecursive<Dialog::PageBase>(state.currentDialog.get(), [&](Dialog::PageBase* pb)
				{
					if(pb->getId() == id)
						matches.add(pb->getInfoObject());

					return false;
				});
			}

			return var(matches);
		}

		var getStyleData(const var::NativeFunctionArgs& args)
		{
			expectArguments(args, 0);

			if(state.currentDialog != nullptr)
			{
				return state.currentDialog->getStyleData().toDynamicObject();
			}

			return var();
		}

		var setStyleData(const var::NativeFunctionArgs& args)
		{
			expectArguments(args, 1);

			if(state.currentDialog != nullptr)
			{
				MarkdownLayout::StyleData sd;
				sd.fromDynamicObject(args.arguments[0], std::bind(&State::loadFont, &state, std::placeholders::_1));
				state.currentDialog->setStyleData(sd);
				
			}

			return var();
		}
	};


	

	eventLogger.sendMessage(getNotificationTypeForCurrentThread(), MessageType::Javascript, "Prepare Javascript execution...");

	auto engine = std::make_unique<JavascriptEngine>();

	if(infoObject.isObject())
		engine->registerNativeObject("element", infoObject.getDynamicObject());

	engine->registerNativeObject("Console", new LogFunction(*this, infoObject));
	engine->registerNativeObject("document", new Dom(*this));
	engine->registerNativeObject("state", globalState.getDynamicObject());
	
	return engine;
}

State::Job::Job(State& rt, const var& obj):
	parent(rt),
	localObj(obj),
	callOnNextEnabled(obj[mpid::CallOnNext])
{
	
}

State::Job::~Job()
{}

void State::Job::postInit()
{
	if(!callOnNextEnabled)
		parent.addJob(this);
}

void State::Job::callOnNext()
{
	if(callOnNextEnabled)
	{
		parent.addJob(this);
		throw CallOnNextAction();
	}
}

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

		if(navigateOnFinish)
		{
			currentDialog->navigate(true);
		}
	}
}

Result State::Job::runJob()
{
	parent.navigateOnFinish |= callOnNextEnabled;


	try
	{
		auto ok = run();
            
		if(auto p = parent.currentDialog)
		{
			MessageManager::callAsync([p, ok]()
			{
				p->repaint();
				//p->errorComponent.setError(ok);
			});
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
				//p->errorComponent.setError(ok);
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
			currentDialog->currentErrorElement = nullptr;
			currentDialog->repaint();
			//currentDialog->errorComponent.setError(Result::ok());
			currentDialog->nextButton.setEnabled(false);
			currentDialog->prevButton.setEnabled(false);
		}
            
		startThread(6);
	}
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
	case Type::RemoveChild: parent.getArray()->insert(index, oldValue);
	default: ;
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
		addAndMakeVisible(dialog = createDialog(state));

		postInit();

		
	}

	dialog->setFinishCallback(closeFunction);
	dialog->setEnableEditMode(false);
	dialog->setBounds(getLocalBounds());
	dialog->showFirstPage();
}
}
}
