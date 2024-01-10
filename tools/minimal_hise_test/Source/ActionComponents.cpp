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
	auto ct = obj[mpid::CallType];
            
	if(ct.isString())
	{
		auto idx = jmax(0, StringArray({"Async", "Sync", "BackgroundThread"}).indexOf(ct.toString()));
            
		callType = (CallType)idx;
	}
	else
		callType = (CallType)(int)ct;
}

void Action::postInit()
{
	switch(callType)
	{
	case CallType::Synchronous: perform(); break;
	case CallType::Asynchronous: MessageManager::callAsync(BIND_MEMBER_FUNCTION_0(Action::perform)); break;
	//case CallType::BackgroundThread: rootDialog.getRunThread().addJob(this); break;
	}
}

void Action::perform()
{
	auto obj = Dialog::getGlobalState(*this, {}, var());
        
	CustomCheckFunction f;
	std::swap(f, cf);
	r = f(this, obj);
}

Result Action::checkGlobalState(var globalState)
{
	return r;
}

Skip::Skip(Dialog& r, int w, const var& obj):
	Action(r, w, obj)
{
	auto direction = r.getCurrentNavigationDirection();
        
	setCustomCheckFunction([direction](Dialog::PageBase* pb, const var& obj)
	{
		auto rd = &pb->getParentDialog();
		MessageManager::callAsync([rd, direction]()
		{
			rd->navigate(direction);
		});
                       
		return Result::ok();
	});
}

DummyWait::WaitJob::WaitJob(State& r, const var& obj):
	Job(r, obj)
{}

Result DummyWait::WaitJob::run()
{
	for(int i = 0; i < 100; i++)
	{
		if(parent.threadShouldExit())
			return Result::fail("aborted");
                
		progress = (double)i / 99.0;
		parent.wait(30);
                

		if(i == 80 && false)
			return abort("**Lost connection**.  \nPlease ensure that your internet connection is stable and click the retry button to resume the download process.");
	}
            
	return Result::ok();

}

Result DummyWait::WaitJob::abort(const String& message)
{
	if(currentPage != nullptr)
	{
		SafeAsyncCall::call<DummyWait>(*currentPage, [](DummyWait& w)
		{
			w.rootDialog.setCurrentErrorPage(&w);
			w.retryButton.setVisible(true);
			w.resized();
		});
	}
            
	return Result::fail(message);
}

DummyWait::DummyWait(Dialog& r, int w, const var& obj):
	Action(r, w, obj),
	retryButton("retry", nullptr, r)
{
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

void DummyWait::paint(Graphics& g)
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

void DummyWait::resized()
{
	auto b = getLocalBounds().reduced(3, 0);
        
	if(label.isNotEmpty())
		b.removeFromLeft(b.getWidth() / 4);
        
	if(retryButton.isVisible())
		retryButton.setBounds(b.removeFromRight(b.getHeight()).withSizeKeepingCentre(24, 24));
        
	progress->setBounds(b);
}
} // PageFactory
} // multipage
} // hise