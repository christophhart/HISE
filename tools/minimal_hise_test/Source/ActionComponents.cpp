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

Action::Action(Dialog& r, int, const var& obj):
	PageBase(r, 0, obj),
	r(Result::ok())
{
	callOnNext = obj[mpid::CallOnNext];

	auto ct = obj[mpid::CallType];
            
	if(ct.isString())
	{
		auto idx = jmax(0, StringArray({"Async", "Sync", "BackgroundThread"}).indexOf(ct.toString()));
            
		callType = (CallType)idx;
	}
	else
		callType = (CallType)(int)ct;

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
		switch(callType)
		{
		case CallType::Synchronous: perform(); break;
		case CallType::Asynchronous: MessageManager::callAsync(BIND_MEMBER_FUNCTION_0(Action::perform)); break;
		//case CallType::BackgroundThread: rootDialog.getRunThread().addJob(this); break;
		}
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

		overlay->setOnClick([this]()
		{
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
{}

Result BackgroundTask::WaitJob::run()
{
	if(currentPage != nullptr)
		return currentPage->performTask(*this);
}



BackgroundTask::BackgroundTask(Dialog& r, int w, const var& obj):
	Action(r, w, obj),
	retryButton("retry", nullptr, r)
{
	positionInfo = r.getPositionInfo(obj);

	addChildComponent(retryButton);
	callType = CallType::BackgroundThread;
        
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
	setSize(w, 32);
}

void BackgroundTask::paint(Graphics& g)
{
	if(label.isNotEmpty())
	{
		auto b = getLocalBounds();
		auto df = Dialog::getDefaultFont(*this);
                
		g.setFont(df.first);
		g.setColour(df.second);
                
		g.drawText(label, b.toFloat().reduced(8.0f, 0.0f), Justification::left);
	}
}

void BackgroundTask::resized()
{
	auto b = getLocalBounds().reduced(3, 0);
        
	if(label.isNotEmpty())
		b.removeFromLeft(positionInfo.getWidthForLabel(getWidth()));
	
	if(retryButton.isVisible())
		retryButton.setBounds(b.removeFromRight(b.getHeight()).withSizeKeepingCentre(24, 24));
        
	progress->setBounds(b.reduced(0, 2));
}
} // PageFactory
} // multipage
} // hise