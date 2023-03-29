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


RangePresets::RangePresets() :
	fileToLoad(getRangePresetFile())
{
	auto xml = XmlDocument::parse(fileToLoad);

	if (xml != nullptr)
	{
		auto v = ValueTree::fromXml(*xml);

		int index = 1;

		for (auto c : v)
		{
			Preset p;
			p.restoreFromValueTree(c);

			p.index = index++;

			presets.add(p);
		}
	}
	else
	{
		createDefaultRange("0-1", { 0.0, 1.0 }, 0.5);
		createDefaultRange("Inverted 0-1", InvertableParameterRange().inverted(), -1.0);
        createDefaultRange("Decibel Gain", {-100.0, 0.0, 0.1}, -12.0);
		createDefaultRange("1-16 steps", { 1.0, 16.0, 1.0 });
		createDefaultRange("Osc LFO", { 0.0, 10.0, 0.0, 1.0 });
		createDefaultRange("Osc Freq", { 20.0, 20000.0, 0.0 }, 1000.0);
		createDefaultRange("Linear 0-20k Hz", { 0.0, 20000.0, 0.0 });
		createDefaultRange("Freq Ratio Harmonics", { 1.0, 16.0, 1.0 });
		createDefaultRange("Freq Ratio Detune Coarse", { 0.5, 2.0, 0.0 }, 1.0);
		createDefaultRange("Freq Ratio Detune Fine", { 1.0 / 1.1, 1.1, 0.0 }, 1.0);

		ValueTree v("Ranges");

		for (const auto& p : presets)
			v.addChild(p.exportAsValueTree(), -1, nullptr);

		auto xml = v.createXml();
		fileToLoad.replaceWithText(xml->createDocument(""));
	}
}

juce::File RangePresets::getRangePresetFile()
{
	return ProjectHandler::getAppDataDirectory(nullptr).getChildFile("RangePresets").withFileExtension("xml");
}

void RangePresets::createDefaultRange(const String& id, InvertableParameterRange d, double midPoint /*= -10000000.0*/)
{
	Preset p;
	p.id = id;
	p.nr = d;

	p.index = presets.size() + 1;

	if (d.getRange().contains(midPoint))
		p.nr.setSkewForCentre(midPoint);

	presets.add(p);
}

RangePresets::~RangePresets()
{

}

void RangePresets::Preset::restoreFromValueTree(const ValueTree& v)
{
	nr = RangeHelpers::getDoubleRange(v);
	id = v[PropertyIds::ID].toString();
}

juce::ValueTree RangePresets::Preset::exportAsValueTree() const
{
	ValueTree v("Range");
	v.setProperty(PropertyIds::ID, id, nullptr);
	RangeHelpers::storeDoubleRange(v, nr, nullptr);
	return v;
}





}

