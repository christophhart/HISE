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

namespace filters
{
using namespace juce;
using namespace hise;

#if 0
FilterNodeGraph::FilterNodeGraph(CoefficientProvider* d, PooledUIUpdater* h) :
	ScriptnodeExtraComponent<CoefficientProvider>(d, h),
	filterGraph(1)
{
	lastCoefficients = {};
	this->addAndMakeVisible(filterGraph);
	filterGraph.addFilter(hise::FilterType::HighPass);

	setSize(256, 100);

	timerCallback();
}

Component* FilterNodeGraph::createExtraComponent(void* p, PooledUIUpdater* h)
{
	auto typed = static_cast<CoefficientProvider*>(p);
	return new FilterNodeGraph(typed, h);
}

bool FilterNodeGraph::coefficientsChanged(const IIRCoefficients& first, const IIRCoefficients& second) const
{
	for (int i = 0; i < 5; i++)
		if (first.coefficients[i] != second.coefficients[i])
			return true;

	return false;
}

void FilterNodeGraph::timerCallback()
{
	if (this->getObject() == nullptr)
		return;

	IIRCoefficients thisCoefficients = this->getObject()->getCoefficients();

	if (coefficientsChanged(lastCoefficients, thisCoefficients))
	{
		lastCoefficients = thisCoefficients;
		filterGraph.setCoefficients(0, this->getObject()->sr, thisCoefficients);
	}
}

void FilterNodeGraph::resized()
{
	if (this->getWidth() > 0)
		filterGraph.setBounds(this->getLocalBounds());
}
#endif

}










}

