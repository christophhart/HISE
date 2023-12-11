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

#pragma once

#if !USE_BACKEND
#error "Only include with USE_BACKEND"
#endif

namespace hise {
using namespace juce;

class BackendRootWindow;

struct PerfettoWebviewer: public Component
{
    PerfettoWebviewer(BackendRootWindow* unused=nullptr);
    ~PerfettoWebviewer();

    static Identifier getGenericPanelId() { RETURN_STATIC_IDENTIFIER("PerfettoWebviewer"); }

	struct Paths: public PathFactory { Path createPath(const String& url) const override; };

    void start(bool shouldStart);
    void resized() override;
    void paint(Graphics& g) override;

    std::function<void()> onStart, onStop;

private:

#if JUCE_USE_WIN_WEBVIEW2
    ScopedPointer<WindowsWebView2WebBrowserComponent> browser;
#elif (JUCE_MAC && USE_BACKEND) || JUCE_LINUX
    ScopedPointer<WebBrowserComponent> browser;
#else
    WebViewData::Ptr data;
    ScopedPointer<WebViewWrapper> browser;
#endif
    
    Paths f;
    HiseShapeButton startButton;
    HiseShapeButton cancelButton;
    
    struct Dragger;

	Dragger* dragger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerfettoWebviewer);
    JUCE_DECLARE_WEAK_REFERENCEABLE(PerfettoWebviewer);
};


} // hise
