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


namespace hise {
namespace multipage {
namespace factory {
using namespace juce;

template <typename T>
void Action::createBasicEditor(T& t, Dialog::PageInfo& rootList, const String& helpText)
{
	rootList.addChild<Type>({
		{ mpid::ID, "Type"},
		{ mpid::Type, T::getStaticId().toString() },
		{ mpid::Help, helpText }
	});
        
	rootList.addChild<TextInput>({
		{ mpid::ID, "ID" },
		{ mpid::Text, "ID" },
		{ mpid::Value, id.toString() },
		{ mpid::Items, rootDialog.getExistingKeysAsItemString() },
		{ mpid::Help, "The ID of the action. This will determine whether the action is tied to a global state value. If not empty, the action will only be performed if the value is not zero." }
	});

	rootList.addChild<Button>({
		{ mpid::ID, "CallOnNext" },
		{ mpid::Text, "CallOnNext" },
		{ mpid::Help, "If enabled, the action will launched when you press the next button (otherwise it will be executed on page load." },
		{ mpid::Value, callOnNext }
	});
}

Action::Action(Dialog& r, int w, const var& obj):
	PageBase(r, 0, obj),
	r(Result::ok())
{
	callOnNext = obj[mpid::CallOnNext];
	
	if(r.isEditModeEnabled())
		setSize(20, 32);
}

void Action::paint(Graphics& g)
{
	g.setColour(Colours::white.withAlpha(0.05f));

	g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(3.0f), 5.0f);

	auto f = Dialog::getDefaultFont(*this);

	String s = "Action: ";

	if(id.isValid())
		s << "if (" << id << ") { " << getDescription() << "; }";
	else
		s << getDescription();

	g.setColour(f.second.withAlpha(0.5f));
	g.setFont(GLOBAL_MONOSPACE_FONT());
	g.drawText(s, getLocalBounds().toFloat(), Justification::centred);
}

void Action::postInit()
{
	init();

	if(!callOnNext)
	{
		perform();
	}
}

void Action::perform()
{
	if(rootDialog.isEditModeEnabled())
	{
		rootDialog.logMessage(MessageType::ActionEvent, "Skip action in edit mode: " + getDescription());
		return;
	}

	auto shouldPerform = getValueFromGlobalState(var(true));

	if(!shouldPerform)
	{
		rootDialog.logMessage(MessageType::ActionEvent, "Skip deactivated action: " + getDescription());
		return;
	}
	
	auto obj = Dialog::getGlobalState(*this, {}, var());
        
	CustomCheckFunction f;
	std::swap(f, cf);

	rootDialog.logMessage(MessageType::ActionEvent, "Perform " + getDescription());
	
	if(f)
		r = f(this, obj);
}

Result Action::checkGlobalState(var globalState)
{
	if(callOnNext)
		perform();

	return r;
}


void Action::editModeChanged(bool isEditMode)
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	PageBase::editModeChanged(isEditMode);

	if(overlay != nullptr)
	{
		overlay->localBoundFunction = [](Component* c)
		{
			return c->getLocalBounds();
		};

		overlay->setOnClick([this](bool isRightClick)
		{
			if(this->showDeletePopup(isRightClick))
				return;

			rootDialog.createEditorInSideTab(infoObject, this, [this](Dialog::PageInfo& r)
			{
				createEditor(r);
			});

#if 0
			auto& l = rootDialog.createModalPopup<List>();

			l.setStateObject(infoObject);
			

			l.setCustomCheckFunction([this](PageBase* b, var obj)
			{
				this->repaint();
				return Result::ok();
			});

			rootDialog.showModalPopup(true);
#endif

		});
	}
#endif
}


ImmediateAction::ImmediateAction(Dialog& r, int w, const var& obj):
	Action(r, w, obj)
{
	setCustomCheckFunction([this](Dialog::PageBase* pb, const var& obj)
	{
		if(id.isValid() && this->skipIfStateIsFalse())
		{
			if(!obj[id])
			{
				rootDialog.logMessage(MessageType::ActionEvent, "Skip because value is false");
				return Result::ok();
			}
				
		}

		if(rootDialog.isEditModeEnabled())
			return Result::ok();

		return this->onAction();
	});
}

Skip::Skip(Dialog& r, int w, const var& obj):
	ImmediateAction(r, w, obj)
{}

void Skip::createEditor(Dialog::PageInfo& rootList)
{
	createBasicEditor(*this, rootList, "An action element that simply skips the page that contains this element. This can be used in order to skip a page with a branch (eg. if one of the options doesn't require additional information.)");

	
}

Result Skip::onAction()
{
	auto rt = &rootDialog;
	auto direction = rt->getCurrentNavigationDirection();
        
	MessageManager::callAsync([rt, direction]()
	{
		rt->navigate(direction);
	});
        
	return Result::ok();
}

void JavascriptFunction::createEditor(Dialog::PageInfo& rootList)
{
	createBasicEditor(*this, rootList, "An action element that will perform a block of Javascript code. You can read / write data from the global state using the `state.variableName` syntax.");

	rootList.addChild<CodeEditor>({
		{ mpid::ID, "Code" },
		{ mpid::Text, "Code" },
		{ mpid::Value, infoObject[mpid::Code] },
		{ mpid::Help, "The JS code that will be evaluated. This is not HiseScript but vanilla JS!  \n> If you want to log something to the console, use `Console.print(message);`." } 
	});
}



Result JavascriptFunction::onAction()
{
	auto code = infoObject[mpid::Code].toString();

	if(code.startsWith("${"))
	{
		code = rootDialog.getState().loadText(code);
	}
        
	auto engine = rootDialog.getState().createJavascriptEngine(infoObject);
	auto ok = engine->execute(code);
	return ok;
}

LinkFileWriter::LinkFileWriter(Dialog& r, int w, const var& obj):
	ImmediateAction(r, w, obj)
{
	auto company = evaluate(mpid::Company);
	auto product = evaluate(mpid::Product);
	auto useGlobal = (bool)rootDialog.getGlobalProperty(mpid::UseGlobalAppData);

	auto f = File::getSpecialLocation(useGlobal ? File::globalApplicationsDirectory : File::userApplicationDataDirectory);

#if JUCE_MAC
        f = f.getChildFile("Application Support");
#endif

	f = f.getChildFile(company).getChildFile(product);

#if JUCE_WINDOWS
	f = f.getChildFile("LinkWindows");
#elif JUCE_MAC
        f = f.getChildFile("LinkOSX");
#else
        f = f.getChildFile("LinkLinux");
#endif

	targetFile = f;
}

void LinkFileWriter::paint(Graphics& g)
{
	g.setColour(Colours::white.withAlpha(0.05f));
	g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(3.0f), 5.0f);

	auto f = Dialog::getDefaultFont(*this);

	String text1, text2;

	text1 << targetFile.getFullPathName() << ":";

	auto targetPath = getValueFromGlobalState(var()).toString();

	if(targetPath.isNotEmpty())
		text2 << "\n" << targetPath;
	else
		text2 << "\n" << "unspecified (" << id.toString() << ")";

	g.setColour(f.second.withAlpha(0.5f));
	g.setFont(GLOBAL_MONOSPACE_FONT());
	
	g.drawText(text1, getLocalBounds().toFloat().reduced(3), Justification::centredTop);
	g.drawText(text2, getLocalBounds().toFloat().reduced(3), Justification::centredBottom);
}

void LinkFileWriter::createEditor(Dialog::PageInfo& rootList)
{
	createBasicEditor(*this, rootList, "Writes the link file to the sample folder to the app data folder.  \n> This takes the global property UseGlobalAppDataFolder into account so make sure that this aligns with your project settings!");
	
	rootList.addChild<TextInput>({
		{ mpid::ID, "Company"},
		{ mpid::Text, "Company" },
		{ mpid::Required, true },
		{ mpid::Value, infoObject[mpid::Company] },
		{ mpid::Help, "Your company name. Used for determining the correct app data folder." }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, "Product"},
		{ mpid::Text, "Product" },
		{ mpid::Required, true },
		{ mpid::Value, infoObject[mpid::Product] },
		{ mpid::Help, "The name of the product. Used for determining the correct app data folder." }
	});
}

Result LinkFileWriter::onAction()
{
	auto linkContent = getValueFromGlobalState(var()).toString();

	if(linkContent.isEmpty())
		return Result::fail("No link file target");

	targetFile.replaceWithText(linkContent);

	return Result::ok();
}


RelativeFileLoader::RelativeFileLoader(Dialog& r, int w, const var& obj):
	ImmediateAction(r, w, obj)
{}

void RelativeFileLoader::createEditor(Dialog::PageInfo& rootList)
{
	createBasicEditor(*this, rootList, "Writes the absolute path of a relative file reference into the state object");

	rootList.addChild<Button>({
		{ mpid::ID, "Required"},
		{ mpid::Text, "Required" },
		{ mpid::Value, infoObject[mpid::Required] },
		{ mpid::Help, "whether the file / directory must exist in order to continue." }
	});

	rootList.addChild<Choice>({
		{ mpid::ID, "SpecialLocation"},
		{ mpid::Text, "SpecialLocation" },
		{ mpid::Required, true },
		{ mpid::Value, infoObject[mpid::SpecialLocation] },
		{ mpid::EmptyText, "Select a special file location" },
		{ mpid::Items, getSpecialLocations().joinIntoString("\n") },
		{ mpid::Help, "The special location type (stolen from the JUCE enum) for the root directory" }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, "RelativePath"},
		{ mpid::Text, "RelativePath" },
		{ mpid::Required, false },
		{ mpid::Value, infoObject[mpid::RelativePath] },
		{ mpid::Help, "An (optional) subpath that will be applied to the file path" }
	});
}

StringArray RelativeFileLoader::getSpecialLocations()
{
	return {
		"userHomeDirectory",
		"userDocumentsDirectory",
		"userDesktopDirectory",
		"userMusicDirectory",
		"userMoviesDirectory",
		"userPicturesDirectory",
		"userApplicationDataDirectory",
		"commonApplicationDataDirectory",
		"commonDocumentsDirectory",
		"tempDirectory",
		"currentExecutableFile",
		"currentApplicationFile",
		"invokedExecutableFile",
		"hostApplicationPath",
		"windowsSystemDirectory",
		"globalApplicationsDirectory",
	};
}

Result RelativeFileLoader::onAction()
{
	auto locString = infoObject[mpid::SpecialLocation].toString();

	auto idx = getSpecialLocations().indexOf(locString);

	if(idx != -1)
	{
		auto f = File::getSpecialLocation((File::SpecialLocationType)idx);

		auto rp = infoObject[mpid::RelativePath].toString();

		if(rp.isNotEmpty())
			f = f.getChildFile(rp);

		if(infoObject[mpid::Required])
		{
			if(!f.existsAsFile() && !f.isDirectory())
			{
				return Result::fail("Can't find " + f.getFullPathName());
			}
		}

		// If it's already set, then we assume that the user changed the location manually...
		if(getValueFromGlobalState("").toString().isEmpty())
			writeState(f.getFullPathName());

		return Result::ok();
	}
}

Launch::Launch(Dialog& r, int w, const var& obj):
	ImmediateAction(r, w, obj)
{
	if(!obj.hasProperty(mpid::CallOnNext))
		callOnNext = true;

	// do not evaluate yet...
	currentLaunchTarget = obj[mpid::Text].toString();
	args = obj[mpid::Args].toString();
}

void Launch::createEditor(Dialog::PageInfo& rootList)
{
	createBasicEditor(*this, rootList, "Shows either a file in the OS file browser or opens an internet browser to load a URL");

	rootList.addChild<TextInput>({
		{ mpid::ID, "Text"},
		{ mpid::Text, "Text" },
		{ mpid::Required, true },
		{ mpid::Value, currentLaunchTarget },
		{ mpid::Help, "The target to be launched. If this is a URL, it will launch the internet browser, if it's a file, it will open the file" }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, "Args"},
		{ mpid::Text, "Args" },
		{ mpid::Required, false },
		{ mpid::Value, args },
		{ mpid::Help, "Additional (command-line) arguments. If this is not empty, the string will be passed as argument" }
	});
}

Result Launch::onAction()
{
	if(isFinished)
		return Result::ok();

	auto t = MarkdownText::getString(currentLaunchTarget, rootDialog);
	auto a = MarkdownText::getString(args, rootDialog).trim();

	if(URL::isProbablyAWebsiteURL(t))
	{
		isFinished = true;
		URL(t).launchInDefaultBrowser();
		return Result::ok();
	}
	
	if(File::isAbsolutePath(t))
	{
		File f(t);

		if(f.existsAsFile() || f.isDirectory())
		{
			if(a.isEmpty())
			{
				isFinished = true;

				f.startAsProcess();

				//f.revealToUser();
				return Result::ok();
			}
			else
			{
				StringArray cmd;
				cmd.add(f.getFullPathName());
				cmd.add(a);
				auto cp = new ChildProcess();
				cp->start(cmd);
				return Result::ok();
			}
		}
		else
		{
			return Result::fail("The file does not exist");
		}
	}

	return Result::ok();
}

String Launch::getDescription() const
{ return "launch(" + MarkdownText::getString(currentLaunchTarget, rootDialog).quoted() + ")"; }

BackgroundTask::WaitJob::WaitJob(State& r, const var& obj):
	Job(r, obj)
{
	
}

Result BackgroundTask::WaitJob::run()
{
	if(currentPage != nullptr)
	{
		if(auto pc = parent.currentDialog)
		{
			if(pc->isEditModeEnabled())
			{
				pc->logMessage(MessageType::ActionEvent, "skip background task in edit mode: " + currentPage->getDescription());
				return Result::ok();
			}
			else if(!currentPage->getValueFromGlobalState(var(true)))
			{
				pc->logMessage(MessageType::ActionEvent, "skip deactivated background task: " + currentPage->getDescription() + " (" + currentPage->id + " == false)");
				return Result::ok();
			}
			else
			{
				pc->logMessage(MessageType::ActionEvent, "Background task: " + currentPage->getDescription());
			}
		}

		try
		{
			auto ok = currentPage->performTask(*this);

			if(ok.failed())
			{
				currentPage->abort(ok.getErrorMessage());
				callOnNextEnabled = true;
				return ok;
			}
		}
		catch(Result& r)
		{
			currentPage->abort(r.getErrorMessage());
			return r;
		}
		
	}
    
    return Result::ok();
}



BackgroundTask::BackgroundTask(Dialog& r, int w, const var& obj):
	Action(r, w, obj),
	retryButton("retry", nullptr, r)
{
	positionInfo.setDefaultPosition(Dialog::PositionInfo::LabelPositioning::Left);

	addChildComponent(retryButton);
        
	job = r.getJob(obj);
	
	if(job == nullptr)
	{
		job = new WaitJob(r.getState(), obj);
	}
        
	dynamic_cast<WaitJob*>(job.get())->currentPage = this;
        
	addAndMakeVisible(progress = new ProgressBar(job->getProgress()));
        
	progress->setOpaque(false);
        
	retryButton.onClick = [this]()
	{
		rootDialog.getState().addJob(job, true);
		retryButton.setVisible(false);
		resized();
	};
        
	label = obj[mpid::Text].toString();
        
	addAndMakeVisible(progress);

	setSize(w, positionInfo.getHeightForComponent(32));
}

void BackgroundTask::paint(Graphics& g)
{
	if(job != nullptr)
		job->updateProgressBar(progress);

	if(label.isNotEmpty())
	{
		auto b = getArea(AreaType::Label);

	    if(rootDialog.isEditModeEnabled())
    		b.reduce(10, 0);

	    auto df = Dialog::getDefaultFont(*this);

	    if(!b.isEmpty())
	    {
			g.setFont(df.first);
			g.setColour(df.second);
	        g.drawText(label, b.toFloat(), Justification::left);
	    }
	}
}

void BackgroundTask::resized()
{
	Action::resized();

	auto b = getArea(AreaType::Component);
	
	if(retryButton.isVisible())
		retryButton.setBounds(b.removeFromRight(b.getHeight()).withSizeKeepingCentre(24, 24));
	
	progress->setBounds(b.reduced(0, 2));
}

void BackgroundTask::postInit()
{
	Action::postInit();

	if(job != nullptr)
		job->postInit();
}

Result BackgroundTask::checkGlobalState(var globalState)
{
	if(callOnNext)
	{
		// make it go through the next time
		callOnNext = false;
		job->callOnNext();
	}
            
	return Action::checkGlobalState(globalState);
}

void BackgroundTask::addSourceTargetEditor(Dialog::PageInfo& rootList)
{
	rootList.addChild<TextInput>(
		{
			{ mpid::ID, "Source" },
			{ mpid::Text, "Source" },
			{ mpid::Help, "The source of the operation. Can be an URL, an absolute path or a variable ID that holds an absolute path / URL" },
			{ mpid::Required, true }, 
			{ mpid::Value, infoObject[mpid::Source] }
		});

	rootList.addChild<TextInput>(
		{
			{ mpid::ID, "Target" },
			{ mpid::Text, "Target" },
			{ mpid::Help, "The target of the operation. Can be an URL, an absolute path or a variable ID that holds an absolute path / URL" },
			{ mpid::Required, true }, 
			{ mpid::Value, infoObject[mpid::Target] }
		});
}

URL BackgroundTask::getSourceURL() const
{
	auto p = evaluate(mpid::Source);

	if(p.isEmpty())
		return URL();
	
	if(URL::isProbablyAWebsiteURL(p))
		return URL(p);

	return URL();
}

Result BackgroundTask::abort(const String& message)
{
	// reset call on next
	if(infoObject[mpid::CallOnNext])
		callOnNext = true;

	auto copy = message;

	rootDialog.logMessage(MessageType::ProgressMessage, "ERROR: " + message);

	SafeAsyncCall::call<BackgroundTask>(*this, [copy](BackgroundTask& w)
	{
		w.rootDialog.setCurrentErrorPage(&w);
		w.setModalHelp(copy);
		w.retryButton.setVisible(true);
		w.resized();
	});
	            
	return Result::fail(message);
}

File BackgroundTask::getFileInternal(const Identifier& id) const
{
	auto p = evaluate(id);

	if(p.isEmpty())
		return File();
	
	if(File::isAbsolutePath(p))
		return File(p);

	return File();
}

LambdaTask::LambdaTask(Dialog& r, int w, const var& obj):
	BackgroundTask(r, w, obj)
{
	lambda = obj[mpid::Function].getNativeFunction();
}

Result LambdaTask::performTask(State::Job& t)
{
	if(!lambda)
	{
		t.setMessage("Empty lambda, simulating...");

		for(int i = 0; i < 30; i++)
		{
			t.getProgress() = (double)i / 30.0;
			t.getThread().wait(50);
		}

		t.getProgress() = 1.0;
		t.setMessage("Done");
            
		return Result::ok();
	}
        
	try
	{
		rootDialog.logMessage(MessageType::ActionEvent, "Call lambda " + id);

		var::NativeFunctionArgs args(rootDialog.getState().globalState, nullptr, 0);

		auto rv = lambda(args);

		if(!rv.isUndefined())
			writeState(rv);

		return Result::ok();
	}
	catch(Result& r)
	{
		return r;
	}
}

String LambdaTask::getDescription() const
{
	return "Lambda Task";
}

void LambdaTask::createEditor(Dialog::PageInfo& rootList)
{
	createBasicEditor(*this, rootList, "An action element that will perform a customizable task.)");

	auto& col = rootList;
        
	col.addChild<TextInput>({
		{ mpid::ID, "Text" },
		{ mpid::Text, "Text" },
		{ mpid::Help, "The label text that will be shown next to the progress bar." }
	});
        
	col.addChild<Choice>({
		{ mpid::ID, "LabelPosition" },
		{ mpid::Text, "LabelPosition" },
		{ mpid::Items, Dialog::PositionInfo::getLabelPositionNames().joinIntoString("\n") },
		{ mpid::Value, "Default" },
		{ mpid::Help, "The position of the text label. This overrides the value of the global layout data. " }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, "Function" },
		{ mpid::Text, "Function" },
		{ mpid::Value, infoObject[mpid::Function] },
		{ mpid::Help, "The full function class name (`Class::functionName`) that will be used as lambda" }
	});
}

HttpRequest::HttpRequest(Dialog& r, int w, const var& obj):
	BackgroundTask(r, w, obj)
{
	if(obj[mpid::Code].toString().isEmpty())
	{
		String templateCode = "function onResponse(status, obj)\n{\n\tif(status == 200)\n\t{\n\t\treturn \"\";\n\t}\n\telse\n\t{\n\t\treturn \"some error\";\n\t}\n};";
		obj.getDynamicObject()->setProperty(mpid::Code, templateCode);
	}

	if(obj[mpid::Parameters].toString().isEmpty())
	{
		obj.getDynamicObject()->setProperty(mpid::Parameters, "{}");
	}
}

Result HttpRequest::performTask(State::Job& t)
{
	auto code = infoObject[mpid::Code].toString();

	auto engine = rootDialog.getState().createJavascriptEngine(infoObject);

	auto r = engine->execute(code);

	if(r.failed())
		return abort(r.getErrorMessage());

	auto hasResponseFunction = engine->getRootObjectProperties().indexOf("onResponse") != -1;

	if(!hasResponseFunction)
		return Result::fail("no `onResponse()` function found");

	auto url = getSourceURL();
	auto parameters = evaluate(mpid::Parameters);

	var pobj;

	r = JSON::parse(parameters, pobj);

	if(r.failed())
		return abort(r.getErrorMessage());

	if(auto o = pobj.getDynamicObject())
	{
		for(const auto& v: o->getProperties())
		{
			url = url.withParameter(v.name.toString(), v.value);
		}
	}
	
	auto usePost = (bool)infoObject[mpid::UsePost];
	auto extraHeaders = evaluate(mpid::ExtraHeaders);
	auto timeout = 5000;

	int statusCode = 0;

	rootDialog.logMessage(MessageType::NetworkEvent, "Calling " + url.toString(true));

	auto now = Time::getMillisecondCounter();

	if(auto stream = url.createInputStream(usePost, nullptr, nullptr, extraHeaders, timeout, nullptr, &statusCode))
	{
		auto response = stream->readEntireStreamAsString();

		var robj;

		auto delta = Time::getMillisecondCounter() - now;
		String lm;
		lm << "HTTP Return code " << String(statusCode) << ": " << String(response.length()) << "bytes (" << String(delta) << "ms)";

		rootDialog.logMessage(MessageType::NetworkEvent, lm);

		if(infoObject[mpid::ParseJSON])
		{
			r = JSON::parse(response, robj);

			if(r.failed())
				return abort(r.getErrorMessage());
		}
		else
			robj = var(response);

		var va[2];
		va[0] = var(statusCode);
		va[1] = var(robj);

		var::NativeFunctionArgs args(var(), va, 2);
		
		auto errorMessage = engine->callFunction("onResponse", args, &r).toString();

		if(r.failed())
			return abort(r.getErrorMessage());

		if(errorMessage.isNotEmpty())
			return abort(errorMessage);

		return Result::ok();
	}
	else
	{
		return abort("No connection");
	}
}

void HttpRequest::createEditor(Dialog::PageInfo& rootList)
{
	createBasicEditor(*this, rootList, "An action element that will perform a HTTP request.)");

	auto& col = rootList;

	col.addChild<TextInput>({
		{ mpid::ID, "Text" },
		{ mpid::Text, "Text" },
		{ mpid::Help, "The label text that will be shown next to the progress bar." }
	});
        
	col.addChild<Choice>({
		{ mpid::ID, "LabelPosition" },
		{ mpid::Text, "LabelPosition" },
		{ mpid::Items, Dialog::PositionInfo::getLabelPositionNames().joinIntoString("\n") },
		{ mpid::Value, "Default" },
		{ mpid::Help, "The position of the text label. This overrides the value of the global layout data. " }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, "Source" },
		{ mpid::Text, "Source" },
		{ mpid::Required, true },
		{ mpid::Help, "The source URL (without parameters)." },
		{ mpid::Value, infoObject[mpid::Source] }
	});

	auto& col2 = rootList;

	col2.addChild<TextInput>({
		{ mpid::ID, "Parameters" },
		{ mpid::Text, "Parameters" },
		{ mpid::Multiline, true },
		{ mpid::Required, true }, 
		{ mpid::Value, infoObject[mpid::Parameters] },
		{ mpid::Help, "The URL parameters as JSON object" }
	});

	col2.addChild<TextInput>({
		{ mpid::ID, "ExtraHeaders" },
		{ mpid::Text, "ExtraHeaders" },
		{ mpid::Multiline, true },
		{ mpid::Value, infoObject[mpid::ExtraHeaders] },
		{ mpid::Help, "The extra headers that are supplied with the HTTP request." }
	});

	auto& col3 = rootList;

	col3.addChild<Button>({
		{ mpid::ID, "UsePost" },
		{ mpid::Text, "UsePost" },
		{ mpid::Required, false },
		{ mpid::Value, infoObject[mpid::UsePost] },
		{ mpid::Help, "Whether to use a POST or GET request." }
	});

	col3.addChild<Button>({
		{ mpid::ID, "ParseJSON" },
		{ mpid::Text, "ParseJSON" },
		{ mpid::Required, false },
		{ mpid::Value, infoObject[mpid::ParseJSON] },
		{ mpid::Help, "Whether to parse the server response as JSON or interpret it as raw string" }
	});

	rootList.addChild<CodeEditor>({
		{ mpid::ID, "Code" },
		{ mpid::Text, "Code" },
		{ mpid::Multiline, true },
		{ mpid::Value, infoObject[mpid::Code] },
		{ mpid::Help, "The extra headers that are supplied with the HTTP request." }
	});
}


DownloadTask::DownloadTask(Dialog& r, int w, const var& obj):
	BackgroundTask(r, w, obj)
{
	usePost = obj[mpid::UsePost];
	extraHeaders = obj[mpid::ExtraHeaders];
}

DownloadTask::~DownloadTask()
{
	ScopedLock sl(downloadLock);
	dt = nullptr;
}



Result DownloadTask::performTask(State::Job& t)
{
	auto targetFile = getTargetFile();

	if(targetFile == File())
	{
		tempFile = new TemporaryFile(id.toString());
		targetFile = tempFile->getFile();
	}

	auto sourceURL = getSourceURL();

	if(sourceURL.isEmpty())
	{
		t.setMessage("Empty download, simulating...");

		for(int i = 0; i < 30; i++)
		{
			t.getProgress() = (double)i / 30.0;
			t.getThread().wait(50);
		}

		t.getProgress() = 1.0;
		t.setMessage("Done");
            
		return Result::ok();
	}

	auto keepTempFile = tempFile != nullptr;

	auto ok = targetFile.getParentDirectory().createDirectory();

	if(ok.failed())
		throw ok;
	
	rootDialog.logMessage(MessageType::NetworkEvent, "Download " + sourceURL.toString(true));
	rootDialog.logMessage(MessageType::NetworkEvent, "Target file: " + targetFile.getFullPathName());



	dt = sourceURL.downloadToFile(targetFile, extraHeaders, nullptr, usePost);

	if(dt != nullptr)
	{
		auto finished = dt->isFinished();
		auto hasError = dt->hadError();

		auto getByteString = [](int64 numBytes)
		{
			if(numBytes < 1024 * 1024)
				return String(numBytes / 1024) + "kB";
			else
				return String(numBytes / (1024 * 1024)) + "MB";
		};

		while(!finished && !hasError)
		{
			if(dt != nullptr)
			{
				ScopedLock sl(downloadLock);

				if(t.getThread().threadShouldExit())
				{
					dt = nullptr;
					tempFile = nullptr;
					return Result::fail("Aborted");
				}

				auto numDownloaded = dt->getLengthDownloaded();
				auto numTotal = dt->getTotalLength();

				if(numTotal > 0)
				{
					t.getProgress() = (double)numDownloaded / (double)numTotal;
				}

				String msg;
				msg << getByteString(numDownloaded) << " / " << getByteString(numTotal);

				t.setMessage(msg);

				finished = dt->isFinished();
				hasError = dt->hadError();
			}
			else
			{
				hasError = true;
				tempFile = nullptr;
				break;
			}

			t.getThread().wait(100);
		}


		

		if(hasError)
		{
			return Result::fail("Download failed");
		}
		else
		{
			rootDialog.logMessage(MessageType::NetworkEvent, "Download complete");
		}
	}

	dt = nullptr;

	// Must be written to the global state so it can pick up a temporary file
	writeState(targetFile.getFullPathName());

	if(keepTempFile)
	{
		rootDialog.logMessage(MessageType::NetworkEvent, "Keep temporary file: " + tempFile->getFile().getFullPathName());
		rootDialog.getState().addTempFile(tempFile.release());
	}

	return Result::ok();
}

void DownloadTask::createEditor(Dialog::PageInfo& rootList)
{
	BackgroundTask::createEditor(rootList);
	createBasicEditor(*this, rootList, "An action element that will download a file from a URL.)");
	
	auto& col = rootList;

	col.addChild<TextInput>({
		{ mpid::ID, "Text" },
		{ mpid::Text, "Text" },
		{ mpid::Help, "The label text that will be shown next to the progress bar." }
	});
        
	col.addChild<Choice>({
		{ mpid::ID, "LabelPosition" },
		{ mpid::Text, "LabelPosition" },
		{ mpid::Items, Dialog::PositionInfo::getLabelPositionNames().joinIntoString("\n") },
		{ mpid::Value, "Default" },
		{ mpid::Help, "The position of the text label. This overrides the value of the global layout data. " }
	});

	addSourceTargetEditor(rootList);

	rootList.addChild<TextInput>({
		{ mpid::ID, "ExtraHeaders" },
		{ mpid::Text, "ExtraHeaders" },
		{ mpid::Required, false },
		{ mpid::Multiline, true },
		{ mpid::Value, infoObject[mpid::ExtraHeaders] },
		{ mpid::Help, "The URL to the file that you want to download." }
	});

	rootList.addChild<Button>({
		{ mpid::ID, "UsePost" },
		{ mpid::Text, "UsePost" },
		{ mpid::Required, false },
		{ mpid::Value, infoObject[mpid::UsePost] },
		{ mpid::Help, "Whether to use a POST or GET request for the download" }
	});
}

String DownloadTask::getDescription() const
{
	return "Download Task";
}

UnzipTask::UnzipTask(Dialog& r, int w, const var& obj):
	BackgroundTask(r, w, obj)
{
	if(obj.hasProperty(mpid::Overwrite))
	{
		overwrite = obj[mpid::Overwrite];
	}
}

Result UnzipTask::performTask(State::Job& t)
{
	auto targetDirectory = getTargetFile();
	auto sourceFile = getSourceFile();

	if(targetDirectory == File())
		return Result::fail("No target directory specified");

	ScopedPointer<MemoryInputStream> mis;
	bool sourceIsFile = sourceFile.existsAsFile();

	if(!sourceIsFile)
	{
		if(auto a = getAsset(mpid::Source))
		{
			if(a->type == Asset::Type::Archive)
			{
				rootDialog.logMessage(MessageType::FileOperation, "Open zip file from asset with filename " + a->filename);
				mis = new MemoryInputStream(a->data, false);
			}
			else
			{
				return Result::fail("Asset type is not an archive");
			}
		}
		else
			return Result::fail("No source archive specified");
	}

	rootDialog.logMessage(MessageType::FileOperation, "Create directory " + targetDirectory.getFullPathName());
	targetDirectory.createDirectory();

	ScopedPointer<ZipFile> zipFile;

	if(sourceIsFile)
	{
		zipFile = new ZipFile(sourceFile);
		rootDialog.logMessage(MessageType::FileOperation, "Open zip file from " + sourceFile.getFullPathName());
	}
	else
	{
		zipFile = new ZipFile(*mis);
		
	}

	for(int i = 0; i < zipFile->getNumEntries(); i++)
	{
		if(t.getThread().threadShouldExit())
			return Result::fail("Aborted");

		t.getProgress() = (double)i / (double)zipFile->getNumEntries();

		zipFile->uncompressEntry(i, targetDirectory, overwrite, nullptr);

		if(rootDialog.getEventLogger().getNumListenersWithClass<EventConsole>() > 0)
		{
			String message;
			auto e = zipFile->getEntry(i);
			message << "  Uncompressing " << targetDirectory.getChildFile(e->filename).getFullPathName();
			message << " (" << String(e->uncompressedSize / 1024) << "kB)";
			rootDialog.logMessage(MessageType::FileOperation, message);
		}
		
		if(t.getThread().threadShouldExit())
			return Result::fail("Aborted");

		if(zipFile->getNumEntries() < 10)
			t.getThread().wait(100);
	}

	rootDialog.logMessage(MessageType::FileOperation, "Unzip operation complete (" + String(zipFile->getNumEntries()) + " files)");

	if(sourceIsFile && (bool)infoObject[mpid::Cleanup])
	{
		if(!sourceFile.deleteFile())
			throw Result::fail("Can't delete source archive");
	}

	return Result::ok();
}

void UnzipTask::createEditor(Dialog::PageInfo& rootList)
{
	BackgroundTask::createEditor(rootList);
	createBasicEditor(*this, rootList, "An action element that will download a file from a URL.)");

	auto& col = rootList;

	col.addChild<TextInput>({
		{ mpid::ID, "Text" },
		{ mpid::Text, "Text" },
		{ mpid::Help, "The label text that will be shown next to the progress bar." }
	});
        
	col.addChild<Choice>({
		{ mpid::ID, "LabelPosition" },
		{ mpid::Text, "LabelPosition" },
		{ mpid::Items, Dialog::PositionInfo::getLabelPositionNames().joinIntoString("\n") },
		{ mpid::Value, "Default" },
		{ mpid::Help, "The position of the text label. This overrides the value of the global layout data. " }
	});

	addSourceTargetEditor(rootList);

	rootList.addChild<Button>({
		{ mpid::ID, "Overwrite" },
		{ mpid::Text, "Overwrite" },
		{ mpid::Required, false },
		{ mpid::Value, overwrite },
		{ mpid::Help, "Whether to use a POST or GET request for the download" }
	});

	rootList.addChild<Button>({
		{ mpid::ID, "Cleanup" },
		{ mpid::Text, "Cleanup" },
		{ mpid::Required, false },
		{ mpid::Value, infoObject[mpid::Cleanup] },
		{ mpid::Help, "Whether to remove the archive after it was extracted successfully" }
	});
}

Result CopyAsset::performTask(State::Job& t)
{
	if(auto a = this->getAsset(mpid::Source))
	{
		auto fn = File(a->filename).getFileName();

		auto targetDir = getTargetFile();

		if(targetDir == File())
		{
			throw Result::fail("No target directory specified");
		}

		auto targetFile = targetDir.getChildFile(fn);

		rootDialog.logMessage(MessageType::FileOperation, "Trying to write asset " + a->id + " to " + targetFile.getFullPathName());

		auto ok = targetDir.createDirectory();

		if(!ok)
		{
			throw Result::fail("Can't create directory " + targetDir.getFullPathName());
		}
            
		if(a->writeToFile(targetFile, &t))
		{
			rootDialog.logMessage(MessageType::FileOperation, "... Done");
			return Result::ok();
		}
		else
			throw Result::fail("Write error: " + targetFile.getFullPathName());
	}
	return Result::fail("Can't find asset");
}

void CopyAsset::createEditor(Dialog::PageInfo& rootList)
{
	BackgroundTask::createEditor(rootList);
	createBasicEditor(*this, rootList, "An action element that will copy an embedded file to a given location.)");

	auto& col = rootList;

	col.addChild<TextInput>({
		{ mpid::ID, "Text" },
		{ mpid::Text, "Text" },
		{ mpid::Help, "The label text that will be shown next to the progress bar." }
	});
        
	col.addChild<Choice>({
		{ mpid::ID, "LabelPosition" },
		{ mpid::Text, "LabelPosition" },
		{ mpid::Items, Dialog::PositionInfo::getLabelPositionNames().joinIntoString("\n") },
		{ mpid::Value, "Default" },
		{ mpid::Help, "The position of the text label. This overrides the value of the global layout data. " }
	});

	addSourceTargetEditor(rootList);

	rootList.addChild<Button>({
		{ mpid::ID, "Overwrite" },
		{ mpid::Text, "Overwrite" },
		{ mpid::Required, false },
		{ mpid::Value, overwrite },
		{ mpid::Help, "Whether the file should overwrite the existing file or not" }
	});
}

HlacDecoder::HlacDecoder(Dialog& r_, int w, const var& obj):
  BackgroundTask(r_, w, obj),
  r(Result::ok())
{
	supportFullDynamics = (bool)obj[mpid::SupportFullDynamics];
	useTotalProgress = (bool)infoObject[mpid::UseTotalProgress];
}

HlacDecoder::~HlacDecoder()
{}

Result HlacDecoder::performTask(State::Job& t)
{
	currentJob = &t;

	hlac::HlacArchiver archiver(&t.getThread());

	double unused1, unused2;

	hlac::HlacArchiver::DecompressData data;
	data.sourceFile = getSourceFile();
	data.targetDirectory = getTargetFile();
	data.debugLogMode = false;
	data.partProgress = &unused1;

	if(useTotalProgress)
	{
		data.progress = &unused2;
		data.totalProgress = &t.getProgress();
	}
	else
	{
		data.progress = &t.getProgress();//&unused2;
		data.totalProgress = &unused2;
	}
	
	data.option = hlac::HlacArchiver::OverwriteOption::OverwriteIfNewer;
	data.supportFullDynamics = supportFullDynamics;

	if(data.sourceFile == File())
		return Result::fail("No source archive specified");

	if(data.targetDirectory == File())
		return Result::fail("No target directory specified");

	archiver.setListener(this);
	archiver.extractSampleData(data);

	currentJob = nullptr;

	return r;
}

void HlacDecoder::logStatusMessage(const String& message)
{
	currentJob->setMessage(message);
}

void HlacDecoder::createEditor(Dialog::PageInfo& rootList)
{
	createBasicEditor(*this, rootList, "An action element that will extract a HR1 archive to the specified target directory.");

	auto& col = rootList;

	col.addChild<TextInput>({
		{ mpid::ID, "Text" },
		{ mpid::Text, "Text" },
		{ mpid::Help, "The label text that will be shown next to the progress bar." }
	});
        
	col.addChild<Choice>({
		{ mpid::ID, "LabelPosition" },
		{ mpid::Text, "LabelPosition" },
		{ mpid::Items, Dialog::PositionInfo::getLabelPositionNames().joinIntoString("\n") },
		{ mpid::Value, "Default" },
		{ mpid::Help, "The position of the text label. This overrides the value of the global layout data. " }
	});

	rootList.addChild<Button>({
		{ mpid::ID, "UseTotalProgress" },
		{ mpid::Text, "UseTotalProgress" },
		{ mpid::Value, useTotalProgress },
		{ mpid::Help, "Whether to display the total progress or the progress for each ch1 file in the progress bar." }
	});

	addSourceTargetEditor(rootList);

	rootList.addChild<Button>({
		{ mpid::ID, "SupportFullDynamics" },
		{ mpid::Text, "SupportFullDynamics" },
		{ mpid::Value, supportFullDynamics },
		{ mpid::Help, "Whether to support the HLAC Full Dynamics mode." }
	});
}

String HlacDecoder::getDescription() const
{ return "HLAC Decoder"; }

DummyWait::DummyWait(Dialog& r, int w, const var& obj):
	BackgroundTask(r, w, obj)
{
	numTodo = (int)obj[mpid::NumTodo];

	if(numTodo == 0)
		numTodo = 100;

	waitTime = (int)obj[mpid::WaitTime];

	if(waitTime < 4)
		waitTime = 30;

	failIndex = (int)obj[mpid::FailIndex];

	if(failIndex == 0)
		failIndex = numTodo + 2;
}

void DummyWait::createEditor(Dialog::PageInfo& rootList)
{
	createBasicEditor(*this, rootList, "An action element that simulates a background task with a progress bar. You can use that during development to simulate the UX before implementing the actual logic.)");
        
	auto& col = rootList;
        
	col.addChild<TextInput>({
		{ mpid::ID, "Text" },
		{ mpid::Text, "Text" },
		{ mpid::Help, "The label text that will be shown next to the progress bar." }
	});
        
	col.addChild<Choice>({
		{ mpid::ID, "LabelPosition" },
		{ mpid::Text, "LabelPosition" },
		{ mpid::Items, Dialog::PositionInfo::getLabelPositionNames().joinIntoString("\n") },
		{ mpid::Value, "Default" },
		{ mpid::Help, "The position of the text label. This overrides the value of the global layout data. " }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, "NumTodo" },
		{ mpid::Text, "NumTodo" },
		{ mpid::Help, "The number of iterations that this action is simulating." }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, "FailIndex" },
		{ mpid::Text, "FailIndex" },
		{ mpid::Help, "The index of the iteration that should cause a failure. If zero or bigger then NumTodo, then the operation succeeds." }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, "WaitTime" },
		{ mpid::Text, "WaitTime" },
		{ mpid::Help, "The duration in milliseconds between each iteration. This makes the duration of the entire task `WaitTime * NumTodo`" }
	});
}

String DummyWait::getDescription() const
{
	return "Dummy Wait";
}

Result DummyWait::performTask(State::Job& t)
{
	if(rootDialog.isEditModeEnabled())
		return Result::ok();
        
	for(int i = 0; i < numTodo; i++)
	{
		if(t.getThread().threadShouldExit())
			return Result::fail("aborted");
	                
		t.getProgress() = (double)i / jmax(1.0, (double)(numTodo-1));
		t.getThread().wait(waitTime);
	                
		if(i == failIndex)
			return abort("**Lost connection**.  \nPlease ensure that your internet connection is stable and click the retry button to resume the download process.");
	}
	            
	return Result::ok();
}

Constants::Constants(Dialog& r, int w, const var& obj):
	PageBase(r, w, obj)
{
	if(r.isEditModeEnabled())
		setSize(w, 32);
	else
		setSize(w, 0);
}

void Constants::postInit()
{
	PageBase::postInit();
	loadConstants();
}

void Constants::paint(Graphics& g)
{
	g.setColour(Colours::white.withAlpha(0.05f));
	g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(3.0f), 5.0f);

	auto f = Dialog::getDefaultFont(*this);

	String s;

	s << "Constants: " << getDescription();
        
	g.setColour(f.second.withAlpha(0.5f));
	g.setFont(GLOBAL_MONOSPACE_FONT());
	g.drawText(s, getLocalBounds().toFloat(), Justification::centred);
}

void Constants::setConstant(const Identifier& id, const var& newValue)
{
	rootDialog.logMessage(MessageType::ValueChangeMessage, "Load constant " + id + " = " + JSON::toString(newValue, true));
	rootDialog.getState().globalState.getDynamicObject()->setProperty(id, newValue);
}

CopyProtection::CopyProtection(Dialog& r, int w, const var& obj):
	Constants(r, w, obj)
{}

void CopyProtection::loadConstants()
{
	setConstant("systemID", juce::OnlineUnlockStatus::MachineIDUtilities::getLocalMachineIDs()[0]);
	setConstant("currentTime", Time::getCurrentTime().toISO8601(true));
	
}

void PluginDirectories::loadConstants()
{
#if JUCE_MAC
	auto auDir = File("~/Library/Audio/Components");
	auto vstDir = File("~/Library/Audio/VST");
	auto vst3Dir = File("~/Library/Audio/VST3");
	auto aaxDir = File("/Library/Application Support/Avid/Audio/Plug-Ins");

	setConstant("auDirectory", auDir.getFullPathName());
	setConstant("vstDirectory", vstDir.getFullPathName());
	setConstant("vst3Directory", vst3Dir.getFullPathName());
	setConstant("aaxDirectory", aaxDir.getFullPathName());
#elif JUCE_WINDOWS
	auto programFiles = File::getSpecialLocation(File::SpecialLocationType::globalApplicationsDirectory);
	auto vstDir = programFiles.getChildFile("VSTPlugins");
	auto vst3Dir = programFiles.getChildFile("Common Files").getChildFile("VST3");
	auto aaxDir = programFiles.getChildFile("Common Files/Avid/Audio/Plug-Ins");
	setConstant("vstDirectory", vstDir.getFullPathName());
	setConstant("vst3Directory", vst3Dir.getFullPathName());
	setConstant("aaxDirectory", aaxDir.getFullPathName());
#endif
}

void OperatingSystem::loadConstants()
{
#if JUCE_WINDOWS
	setConstant("WINDOWS", true);
	setConstant("MAC_OS", false);
	setConstant("LINUX", false);
	setConstant("OS", (int)Asset::TargetOS::Windows);
#elif JUCE_LINUX
    setConstant("WINDOWS", false);
	setConstant("MAC_OS", false);
    setConstant("LINUX", true);
	setConstant("OS", (int)Asset::TargetOS::Linux);
#elif JUCE_MAC
    setConstant("WINDOWS", false);
    setConstant("MAC_OS", true);
	setConstant("LINUX", false);
	setConstant("OS", (int)Asset::TargetOS::macOS);
#endif
}

void FileLogger::createEditor(Dialog::PageInfo& rootList)
{
	rootList.addChild<Type>({
		{ mpid::ID, "Type"},
		{ mpid::Type, FileLogger::getStaticId().toString() },
		{ mpid::Help, "Adding this will start logging any events to a customizable file location so you can track the process" }
	});
        
	rootList.addChild<TextInput>({
		{ mpid::ID, "Filename" },
		{ mpid::Text, "Filename" },
		{ mpid::Value, infoObject[mpid::Filename].toString() },
		{ mpid::Help, "The target name. This can be a combination of a state variable and a child path, eg. `$targetDirectory/Logfile.txt`" }
	});
}

void ProjectInfo::loadConstants()
{
	setConstant("company", rootDialog.getGlobalProperty(mpid::Company));
	setConstant("product", rootDialog.getGlobalProperty(mpid::ProjectName));
	setConstant("version", rootDialog.getGlobalProperty(mpid::Version));
}
} // PageFactory
} // multipage
} // hise
