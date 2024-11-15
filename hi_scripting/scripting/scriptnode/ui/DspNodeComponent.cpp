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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;


DefaultParameterNodeComponent::DefaultParameterNodeComponent(NodeBase* node) :
	NodeComponent(node)
{
	parameterListener.setCallback(node->getParameterTree(), valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(DefaultParameterNodeComponent::updateSliders));

	updateSliders(node->getParameterTree(), false);
}


void DefaultParameterNodeComponent::resized()
{
	auto rowHeight = 48 + 28;
	NodeComponent::resized();

	auto b = getLocalBounds();
	b = b.reduced(UIValues::NodeMargin);
	b.removeFromTop(UIValues::HeaderHeight);

	if (embeddedNetworkBar != nullptr)
		b.removeFromTop(24);

	if (extraComponent != nullptr)
	{
		extraComponent->setBounds(b.removeFromTop(extraComponent->getHeight()));
		b.removeFromTop(UIValues::NodeMargin);
	}
	
	

	int numPerRow = jlimit(1, jmax(sliders.size(), 1), b.getWidth() / 100);
	int numColumns = jmax(1, (b.getHeight() + 10) / rowHeight);

	auto staticIntend = 0;

	if (numColumns == 2)
	{
		numPerRow = (int)hmath::ceil((float)sliders.size() / 2.0f);
	}

	staticIntend = (b.getWidth() - numPerRow * 100) / 2;
		

	auto intendOddRows = (sliders.size() % jmax(1, numPerRow)) != 0;

	auto rowIndex = 0;

    auto row = b.removeFromTop(rowHeight);
    row.removeFromLeft(staticIntend);
    row.removeFromRight(staticIntend);
	
    for(auto s: sliders)
    {
        auto sliderBounds = row.removeFromLeft(100);
        
        if(sliderBounds.getWidth() < 100)
        {
            rowIndex++;
            row = b.removeFromTop(rowHeight);
        
            auto intend = staticIntend;
            
            if (intendOddRows && (rowIndex % 2 != 0))
                intend += 50;
            
            row.removeFromLeft(intend);
            row.removeFromRight(staticIntend);
			
            sliderBounds = row.removeFromLeft(100);
        }

		if(b.getHeight() > 0)
			sliderBounds.removeFromBottom(10);

		s->setBounds(sliderBounds);
    }
}






}

