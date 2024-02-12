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
	auto obj = Dialog::getGlobalState(*this, {}, var());
        
	CustomCheckFunction f;
	std::swap(f, cf);

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

			auto& l = rootDialog.createModalPopup<List>();

			l.setStateObject(infoObject);
			createEditor(l);

			l.setCustomCheckFunction([this](PageBase* b, var obj)
			{
				this->repaint();
				return Result::ok();
			});

			rootDialog.showModalPopup(true);

		});
	}
}



BackgroundTask::WaitJob::WaitJob(State& r, const var& obj):
	Job(r, obj)
{
	
}

Result BackgroundTask::WaitJob::run()
{
	if(currentPage != nullptr)
		return currentPage->performTask(*this);
    
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

void LambdaTask::createEditor(Dialog::PageInfo& rootList)
{
	createBasicEditor(*this, rootList, "An action element that will perform a customizable task.)");

	auto& col = rootList.addChild<factory::Column>();
        
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

void DummyWait::createEditor(Dialog::PageInfo& rootList)
{
	createBasicEditor(*this, rootList, "An action element that simulates a background task with a progress bar. You can use that during development to simulate the UX before implementing the actual logic.)");
        
	auto& col = rootList.addChild<factory::Column>();
        
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
} // PageFactory
} // multipage
} // hise
