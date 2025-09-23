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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;




String RangeHelpers::toDisplayString(InvertableParameterRange d)
{
	String s = "[";

	auto numDigits = 0;
	
	if (d.rng.interval != 0)
	{
		numDigits = -1 * log10(d.rng.interval);
	}
	else
	{
		auto range = d.rng.getRange().getLength();

		numDigits = range > 2.0 ? 1 : 2;
	}

	auto getString = [numDigits](double d)
	{
		return String(d, numDigits, false);
	};

	auto start = getString(d.inv ? d.rng.end : d.rng.start);
	auto end = getString(d.inv ? d.rng.start : d.rng.end);
	auto mid = getString(d.convertFrom0to1(0.5, true));

	s << start << " - " << mid << " - " << end << "]";

	return s;
}

bool RangeHelpers::isRangeId(const Identifier& id)
{
	using namespace PropertyIds;
	return id == MinValue || id == MaxValue || id == StepSize || id == SkewFactor;
}


bool RangeHelpers::isBypassIdentity(InvertableParameterRange d)
{
	if (d.rng.start == 0.5 && d.rng.end == 1.0 && d.rng.skew == 1.0 && !d.inv)
		return true;

	return false;
}

bool RangeHelpers::isIdentity(InvertableParameterRange d)
{
	if (d.rng.start == 0.0 && d.rng.end == 1.0 && d.rng.skew == 1.0 && !d.inv)
		return true;

	return false;
}


void RangeHelpers::removeRangeProperties(ValueTree v, UndoManager* um, IdSet set)
{
	for (auto id : getRangeIds(false, set))
		v.removeProperty(id, um);

	v.removeProperty(Identifier("Enabled"), um);
}

juce::Array<juce::Identifier> RangeHelpers::getRangeIds(bool includeValue, IdSet rangeIdSet)
{
	using namespace PropertyIds;

	Array<Identifier> rangeIds;

	switch(rangeIdSet)
	{
	case IdSet::scriptnode:			rangeIds = { MinValue, MaxValue, StepSize, SkewFactor }; break;
	case IdSet::ScriptComponents:	rangeIds = { Identifier("min"), Identifier("max"), Identifier("stepSize"), Identifier("middlePosition") }; break;
	case IdSet::MidiAutomation:     rangeIds = { Identifier("Start"), Identifier("End"), Identifier("Interval"), Identifier("Skew") }; break;
	case IdSet::MidiAutomationFull: rangeIds = { Identifier("FullStart"), Identifier("FullEnd"), Identifier("Interval"), Identifier("Skew") }; break;
    case IdSet::numIdSets:
    default: break;
	}

	if (includeValue)
		rangeIds.add(Value);

	return rangeIds;
}


bool RangeHelpers::isInverted(const ValueTree& v, IdSet rangeIdSet)
{
    if (!v.isValid())
		return false;

	using namespace PropertyIds;
    
    if (v.getType() == Connection || v.getType() == ModulationTarget)
    {
        // this must not happen (the connection does not have a range anymore, but just take the
        // target's parameter range
        jassertfalse;
    }

	if(rangeIdSet == IdSet::scriptnode)
	{
		auto max = (double)v[ri(rangeIdSet, RangeIdentifier::Maximum)];
		auto min = (double)v[ri(rangeIdSet, RangeIdentifier::Minimum)];

		return min > max;
	}
	else
	{
		return v[Inverted];
	}
}

void RangeHelpers::storeDoubleRange(ValueTree& d, InvertableParameterRange r, UndoManager* um, IdSet rangeIdSet)
{
	using namespace PropertyIds;
    
    if (d.getType() == Connection || d.getType() == ModulationTarget)
    {
        // this must not happen (the connection does not have a range anymore, but just take the
        // target's parameter range
        jassertfalse;
    }

	auto maxId = ri(rangeIdSet, RangeIdentifier::Maximum);
	auto minId = ri(rangeIdSet, RangeIdentifier::Minimum);

	if(rangeIdSet == IdSet::scriptnode) // we only use the swapped values in the scriptnode set, otherwise we'll set the Inverted property
	{
		d.setProperty(r.inv ? maxId : minId, r.rng.start, um);
		d.setProperty(r.inv ? minId : maxId, r.rng.end, um);
	}
	else
	{
		d.setProperty(minId, r.rng.start, um);
		d.setProperty(maxId, r.rng.end, um);
		d.setProperty(Inverted, r.inv, um);
	}
	
	d.setProperty(ri(rangeIdSet, RangeIdentifier::StepSize), r.rng.interval, um);

	if(rangeIdSet == IdSet::ScriptComponents)
	{
		if(r.rng.skew != 1.0)
		{
			auto midPosition = r.convertFrom0to1(0.5, false);
			d.setProperty(ri(rangeIdSet, RangeIdentifier::SkewFactor), midPosition, um);
		}
	}
	else
	{
		d.setProperty(ri(rangeIdSet, RangeIdentifier::SkewFactor), r.rng.skew, um);
	}

	
}

std::pair<bool, double> RangeHelpers::getDefaultValue(const var& obj)
{
	double defaultValue = 0.0;

	// the default value is not part of the RangeHelper::IdSet so we must fetch this manually
	if(obj.hasProperty(PropertyIds::DefaultValue))
		defaultValue = (double)obj[PropertyIds::DefaultValue];
	else if(obj.hasProperty("defaultValue"))
		defaultValue = (double)obj["defaultValue"];
	else
		return { false, 0.0 };
		
	FloatSanitizers::sanitizeDoubleNumber(defaultValue);

	auto idSet = getIdSetForJSON(obj);
	auto rng = getDoubleRange(obj, idSet);

	if(defaultValue < rng.rng.start)
		defaultValue = rng.getRange().getStart();
	if(defaultValue > rng.rng.end)
		defaultValue = rng.getRange().getEnd();

	return {true, defaultValue};
}

void RangeHelpers::storeDoubleRange(var& obj, InvertableParameterRange r, IdSet rangeIdSet)
{
	using namespace PropertyIds;

	if(obj.getDynamicObject() == nullptr)
	{
		obj = var(new DynamicObject());
	}

	auto d = obj.getDynamicObject();
	jassert(d != nullptr);

	auto maxId = ri(rangeIdSet, RangeIdentifier::Maximum);
	auto minId = ri(rangeIdSet, RangeIdentifier::Minimum);

	if (rangeIdSet == IdSet::scriptnode) // we only use the swapped values in the scriptnode set, otherwise we'll set the Inverted property
	{
		d->setProperty(r.inv ? maxId : minId, r.rng.start);
		d->setProperty(r.inv ? minId : maxId, r.rng.end);
	}
	else
	{
		d->setProperty(minId, r.rng.start);
		d->setProperty(maxId, r.rng.end);
		d->setProperty(Inverted, r.inv);
	}

	d->setProperty(ri(rangeIdSet, RangeIdentifier::StepSize), r.rng.interval);

	if(rangeIdSet == IdSet::ScriptComponents)
	{
		if(r.rng.skew != 1.0)
		{
			auto midPosition = r.convertFrom0to1(0.5, false);
			d->setProperty(ri(rangeIdSet, RangeIdentifier::SkewFactor), midPosition);
		}
	}
	else
	{
		d->setProperty(ri(rangeIdSet, RangeIdentifier::SkewFactor), r.rng.skew);
	}
}

bool RangeHelpers::equalsWithError(const InvertableParameterRange& r1, const InvertableParameterRange& r2, double maxError)
{
	if (isEqual(r1, r2))
		return true;

    if(r1.inv != r2.inv)
        return false;
    
	auto startError = hmath::abs(r1.getRange().getStart() - r2.getRange().getStart());
	auto endError = hmath::abs(r1.getRange().getEnd() - r2.getRange().getEnd());
	auto skewError = hmath::abs(r1.rng.skew - r2.rng.skew);
	auto intervalError = hmath::abs(r1.rng.interval - r2.rng.interval);

	auto thisError = jmax(startError, endError, skewError, intervalError);

	return thisError < hmath::abs(maxError);

}

bool RangeHelpers::isEqual(const InvertableParameterRange& r1, const InvertableParameterRange& r2)
{
	return  r1.rng.start == r2.rng.start &&
		r1.rng.end == r2.rng.end &&
		r1.rng.skew == r2.rng.skew &&
		r1.rng.interval == r2.rng.interval &&
		r1.inv == r2.inv;
}

scriptnode::InvertableParameterRange RangeHelpers::getDoubleRange(const ValueTree& t, IdSet set)
{
	using namespace PropertyIds;

	InvertableParameterRange r;

	if (t.getType() == Connection || t.getType() == ModulationTarget)
	{
		// this must not happen (the connection does not have a range anymore, but just take the 
		// target's parameter range
		jassertfalse;
	}

	auto minValue = (double)t.getProperty(ri(set, RangeIdentifier::Minimum), 0.0);
	auto maxValue = (double)t.getProperty(ri(set, RangeIdentifier::Maximum), 1.0);

	if (minValue == maxValue)
		maxValue += 0.01;

	if (set == IdSet::scriptnode)
	{
		if (minValue > maxValue)
		{
			std::swap(minValue, maxValue);
			r.inv = true;
		}
	}
	else
		r.inv = t[Inverted];
	
	r.rng.start = minValue;
	r.rng.end = maxValue;

	auto skewId = ri(set, RangeIdentifier::SkewFactor);
	auto stepId = ri(set, RangeIdentifier::StepSize);

	if(t.hasProperty(stepId))
		r.rng.interval = jlimit(0.0, 1.0, (double)t[stepId]);

	if(t.hasProperty(skewId))
	{
		if(set == IdSet::ScriptComponents)
		{
			auto midPos = (double)t[skewId];

			if(midPos > r.rng.start && midPos < r.rng.end)
				r.rng.setSkewForCentre(jlimit(r.rng.start, r.rng.end, midPos));
		}
		else
		{
			r.rng.skew = jlimit(0.001, 100.0, (double)t[skewId]);
		}
	}

	r.checkIfIdentity();

	return r;
}

scriptnode::InvertableParameterRange RangeHelpers::getDoubleRange(const var& obj, IdSet set)
{
	ValueTree v(PropertyIds::ID);

	for (auto r : getRangeIds(false, set))
	{
		if (obj.hasProperty(r))
			v.setProperty(r, obj[r], nullptr);
	}

	return getDoubleRange(v, set);
}

RangeHelpers::IdSet RangeHelpers::getIdSetForJSON(const var& obj)
{
	Array<Identifier> thisIds;

	if(auto o = obj.getDynamicObject())
	{
		for(auto& nv: o->getProperties())
			thisIds.add(nv.name);
	}

	for(const auto& id: thisIds)
	{
		if(getRangeIds(false, IdSet::scriptnode).contains(id))
			return IdSet::scriptnode;
		if(getRangeIds(false, IdSet::ScriptComponents).contains(id))
			return IdSet::ScriptComponents;
		if(getRangeIds(false, IdSet::MidiAutomation).contains(id))
			return IdSet::MidiAutomation;
		if(getRangeIds(false, IdSet::MidiAutomationFull).contains(id))
			return IdSet::MidiAutomationFull;
	}

	return IdSet::scriptnode;
}

namespace parameter
{

	data::data(const String& id_)
	{
		info.setId(id_);
		info.setRange({ 0.0, 1.0 });
		info.ok = id_.isNotEmpty();
	}

	data::data(const String& id_, InvertableParameterRange r)
	{
		info.setId(id_);
		info.setRange(r);
		info.ok = id_.isNotEmpty();
	}

	data::data()
	{

	}

	juce::ValueTree data::createValueTree() const
	{
		ValueTree p(PropertyIds::Parameter);

		RangeHelpers::storeDoubleRange(p, info.toRange(), nullptr);

		p.setProperty(PropertyIds::ID, info.getId(), nullptr);
		p.setProperty(PropertyIds::TextToValueConverter, pod::getTextValueConverterNames()[(int)info.textConverter], nullptr);

		PageInfo pi(info);

		p.setProperty(PropertyIds::Page, pi.page, nullptr);
		p.setProperty(PropertyIds::SubGroup, pi.group, nullptr);
		p.setProperty(PropertyIds::Value, info.defaultValue, nullptr);
		p.setProperty(PropertyIds::DefaultValue, info.defaultValue, nullptr);
		return p;
	}

	data data::withRange(InvertableParameterRange r)
	{
		data copy(*this);
		copy.info.setRange(r);
		return copy;
	}

	hise::ValueToTextConverter data::getValueToTextConverter() const
	{
		if (!parameterNames.isEmpty())
		{
			return ValueToTextConverter::createForOptions(parameterNames);
		}

		switch (info.textConverter)
		{
		case pod::Frequency: return ValueToTextConverter::createForMode("Frequency");
		case pod::Time:		 return ValueToTextConverter::createForMode("Time");
		case pod::TempoSync: return ValueToTextConverter::createForMode("TempoSync");
		case pod::Pan:		 return ValueToTextConverter::createForMode("Pan");
		case pod::NormalizedPercentage:
			return ValueToTextConverter::createForMode("NormalizedPercentage");
		case pod::Decibel:   return ValueToTextConverter::createForMode("Decibel");
		case pod::Undefined:
		case pod::numTextValueConverters:
		default: return {};
		}
	}

	void data::setParameterValueNames(const StringArray& valueNames)
	{
		parameterNames = valueNames;

		if (valueNames.size() > 1)
			setRange({ 0.0, (double)valueNames.size() - 1.0, 1.0 });
	}

	void data::init()
	{
		callback(info.defaultValue);
	}

	dynamic::dynamic() :
		f(nullptr),
		obj(nullptr)
	{

	}

	void dynamic::call(double v) const
	{
		if (f != nullptr && obj != nullptr)
			f(getObjectPtr(), v);
	}

	void dynamic::referTo(void* p, Function f_)
	{
		f = f_;
		obj = p;
	}

	void dynamic::operator()(double v) const
	{
		call(v);
	}

	void* dynamic::getObjectPtr() const
	{
		return obj;
	}



	encoder::encoder(ValueTree& v)
	{
		for (auto c : v)
			items.add(pod(c));
	}

	encoder::encoder(const MemoryBlock& m)
	{
		parseItems(m);
	}

	void encoder::parseItems(const MemoryBlock& mb)
	{
		MemoryInputStream mis(mb, true);

		while (!mis.isExhausted())
		{
			pod item(mis);

			if (item.ok)
				items.add(item);
		}
	}

	MemoryBlock encoder::writeItems()
	{
		MemoryBlock data;

		MemoryOutputStream mos(data, false);

		for (auto& d : items)
			d.writeToStream(mos);

		int numToPad = 4 - mos.getPosition() % 4;

		for (int i = 0; i < numToPad; i++)
			mos.writeByte(0);

		mos.flush();

		return data;
	}

	juce::StringArray pod::getTextValueConverterNames()
	{
		return {
			"Undefined",
			"Frequency",
			"Time",
			"TempoSync",
			"Pan",
			"NormalizedPercentage",
			"Decibel",
			"Undefined"
		};
	}

	pod::pod(const ValueTree& v)
	{
		clearParameterName();
		clearPageInfo();

		index = v.getParent().indexOf(v);
		ok = setId(v[PropertyIds::ID].toString());

		setPageGroup(v[PropertyIds::Page].toString(), v[PropertyIds::SubGroup].toString());

		auto range = RangeHelpers::getDoubleRange(v);
		min = (DataType)range.rng.start;
		max = (DataType)range.rng.end;
		inverted = range.inv;
		skew = (DataType)range.rng.skew;
		interval = (DataType)range.rng.interval;
		defaultValue = (DataType)v[PropertyIds::Value];

		auto tv = getTextValueConverterNames().indexOf(v[PropertyIds::TextToValueConverter].toString());

		if(tv != -1)
		{
			textConverter = (TextValueConverters)tv;
		}
	}

	pod::pod(MemoryInputStream& mis)
	{
		clearParameterName();
		clearPageInfo();

		auto safe = mis.readByte();

		if (safe == 92)
		{
			ok = true;

			textConverter = (TextValueConverters)mis.readByte();
			index = mis.readInt();
			auto s = mis.readString();

			ok = setId(s);

			PageInfo pi(mis);
			setPageGroup(pi.page, pi.group);

			min = mis.readFloat();
			max = mis.readFloat();
			defaultValue = mis.readFloat();
			skew = mis.readFloat();
			interval = mis.readFloat();
		}
	}

	String pod::toString()
	{
		String s;
		String nl;

		s << "index: " << index << nl;
		s << "id: " << parameterName << nl;
		s << "min: " << min << nl;
		s << "max: " << max << nl;
		s << "converter: " << getTextValueConverterNames()[(int)textConverter];
		
		PageInfo pd(*this);

		if(pd)
		{
			s << "page: " << pd.page << nl;
			s << "group: " << pd.group << nl;
		}

		return s;
	}

	bool pod::setPageName(const String& newPageName)
	{
		if (newPageName.isNotEmpty() && isPositiveAndBelow(newPageName.length(), MaxPageNameLength))
		{
			memcpy(pageName, newPageName.getCharPointer().getAddress(), newPageName.length());
			return true;
		}
		else
		{
			memset(pageName, 0, MaxPageNameLength);
			return false;
		}
	}

	bool pod::setGroupName(const String& newGroupName)
	{
		if (newGroupName.isNotEmpty() && isPositiveAndBelow(newGroupName.length(), MaxPageNameLength))
		{
			memcpy(subGroupName, newGroupName.getCharPointer().getAddress(), newGroupName.length());
			return true;
		}
		else
		{
			memset(subGroupName, 0, MaxPageNameLength);
			return false;
		}
	}

	void pod::clearParameterName()
	{
		memset(parameterName, 0, MaxParameterNameLength);
	}

	void pod::clearPageInfo()
	{
		memset(pageName, 0, MaxPageNameLength);
		memset(subGroupName, 0, MaxPageNameLength);
	}

	void pod::setRange(const InvertableParameterRange& r)
	{
		min =	   r.rng.start;
		max =	   r.rng.end;
		interval = r.rng.interval;
		skew =	   r.rng.skew;
		inverted = r.inv;
	}

	scriptnode::InvertableParameterRange pod::toRange() const
	{
		InvertableParameterRange r;
		r.rng.start = min;
		r.rng.end = max;
		r.rng.skew = skew;
		r.rng.interval = interval;
		r.inv = inverted;

		return r;
	}

	bool pod::setId(const String& id)
	{
		if (id.isNotEmpty() && isPositiveAndBelow(id.length(), MaxParameterNameLength))
		{
			memcpy(parameterName, id.getCharPointer().getAddress(), id.length());
			return true;
		}
		else
			clearParameterName();

		return false;
	}

	
	void pod::setPageGroup(const String& p, const String& g)
	{
		clearPageInfo();

		if (p.isNotEmpty() && isPositiveAndBelow(p.length(), MaxPageNameLength))
			memcpy(pageName, p.getCharPointer().getAddress(), p.length());
		if (g.isNotEmpty() && isPositiveAndBelow(g.length(), MaxPageNameLength))
			memcpy(subGroupName, g.getCharPointer().getAddress(), g.length());
	}

	void pod::writeToStream(MemoryOutputStream& b)
	{
		b.writeByte(92);
		b.writeByte((uint8)textConverter);
		b.writeInt(index);

		String id(parameterName);

		b.writeString(id);

		PageInfo pd(*this);
		pd.writeToStream(b);

		b.writeFloat(min);
		b.writeFloat(max);
		b.writeFloat(defaultValue);
		b.writeFloat(skew);
		b.writeFloat(interval);
	}


}

InvertableParameterRange::InvertableParameterRange(const ValueTree& v)
{
	*this = RangeHelpers::getDoubleRange(v);
}

double InvertableParameterRange::convertFrom0to1(double input, bool applyInversion) const
{
	if (isIdentity)
		return input;

    auto inversionFactor = (double)(int)(applyInversion && inv);
    
    // branchless like a pro...
    input = input * (1.0 - inversionFactor) + (1.0 - input) * inversionFactor;
    
	if (rng.skew != 1.0)
		return ranges::RangeBase<double>::from0To1Skew(rng.start, rng.end, rng.skew, input);
	else if (rng.interval != 0.0)
		return ranges::RangeBase<double>::from0To1Step(rng.start, rng.end, rng.interval, input);
	else
		return ranges::RangeBase<double>::from0To1(rng.start, rng.end, input);
}

double InvertableParameterRange::convertTo0to1(double input, bool applyInversion) const
{
	if (isIdentity)
		return input;

    double v;
    
	if (rng.skew != 1.0)
		v = ranges::RangeBase<double>::to0To1Skew(rng.start, rng.end, rng.skew, input);
	else if (rng.interval != 0.0)
		v = ranges::RangeBase<double>::to0To1Step(rng.start, rng.end, rng.interval, input);
	else
		v = ranges::RangeBase<double>::to0To1(rng.start, rng.end, input);
    
    auto inversionFactor = (double)(int)(applyInversion && inv);
    
    // branchless like a pro...
    v = v * (1.0 - inversionFactor) + (1.0 - v) * inversionFactor;
    
    return v;
}

bool InvertableParameterRange::operator==(const InvertableParameterRange& other) const
{
	return RangeHelpers::isEqual(*this, other);
}

void InvertableParameterRange::checkIfIdentity()
{
	isIdentity = RangeHelpers::isIdentity(*this);
}

void InvertableParameterRange::store(ValueTree& v, UndoManager* um)
{
	RangeHelpers::storeDoubleRange(v, *this, um);
}

OSCConnectionData::OSCConnectionData(const var& data /*= var()*/)
{
	domain = data.getProperty("Domain", "/hise_osc_receiver");

	if (!domain.startsWithChar('/'))
		domain = "/" + domain;

	if (domain.endsWithChar('/'))
		domain = domain.upToLastOccurrenceOf("/", false, false);

	sourceURL = data.getProperty("SourceURL", "127.0.0.1");
	sourcePort = data.getProperty("SourcePort", 9000);
	targetURL = data.getProperty("TargetURL", "127.0.0.1");
	targetPort = data.getProperty("TargetPort", -1);
	isReadOnly = targetPort == -1;

	if (data.hasProperty("Parameters"))
	{
		if (auto obj = data["Parameters"].getDynamicObject())
		{
			for (const auto& nv : obj->getProperties())
			{
				NamedRange nr;
				nr.id = nv.name.toString();
				nr.rng = RangeHelpers::getDoubleRange(nv.value);
				inputRanges.add(nr);
			}
		}
	}
}

bool OSCConnectionData::operator==(const OSCConnectionData& otherData) const
{
	auto sameData = domain == otherData.domain &&
					sourceURL == otherData.sourceURL &&
					sourcePort == otherData.sourcePort &&
					targetURL == otherData.targetURL &&
					targetPort == otherData.targetPort &&
					isReadOnly == otherData.isReadOnly;
	
	if (sameData)
	{
		auto numToCheck = jmax(inputRanges.size(), otherData.inputRanges.size());

		for (int i = 0; i < numToCheck; i++)
		{
			if (!(inputRanges[i] == otherData.inputRanges[i]))
				return false;
		}

		return true;
	}

	return false;
}

PageInfo::Tree::Tree(const ParameterDataList& list)
{
	for (const auto& p : list)
		hasLayout |= (bool)PageInfo(p.info);

	if (!hasLayout)
		flatList = list;
	else
	{
		String currentPage = "";
		String currentGroup = "";

		for (const auto& p : list)
		{
			PageInfo tp(p.info);

			if (tp.isPage())
			{
				currentPage = tp.page;
			}

			if (tp.isGroup())
			{
				currentGroup = tp.group;
				hasTags = true;
			}

			getBucket(currentPage, currentGroup).add(p);
		}

		for (auto outerIt = pageTree.begin(); outerIt != pageTree.end(); )
		{
			auto& innerMap = outerIt->second;

			// Remove empty inner maps (if those are maps themselves)
			for (auto innerIt = innerMap.begin(); innerIt != innerMap.end(); )
			{
				if (innerIt->second.isEmpty())
					innerIt = innerMap.erase(innerIt);
				else
					++innerIt;
			}

			// Now remove the outer entry if its inner map is empty
			if (innerMap.empty())
				outerIt = pageTree.erase(outerIt);
			else
				++outerIt;
		}
	}
}

int PageInfo::Tree::getNumMaxSlidersPerPage() const
{
	if (hasPageLayout())
	{
		auto numParameters = 0;

		for (auto& p : pageTree)
		{
			auto numOnPage = 0;

			for (auto& g : p.second)
				numOnPage += g.second.size();

			numParameters = jmax(numParameters, numOnPage);
		}

		return numParameters;
	}
	else
	{
		return flatList.size();
	}
}

String PageInfo::Tree::getGroupIdForParameter(int parameterIndex) const
{
	for (const auto& p : pageTree)
	{
		for (const auto& g : p.second)
		{
			for (const auto& pr : g.second)
			{
				if (pr.info.index == parameterIndex)
					return g.first;
			}
		}
	}

	return {};
}

juce::StringArray PageInfo::Tree::getPageNames() const
{
	StringArray sa;

	for (const auto& p : pageTree)
		sa.add(p.first);

	return sa;
}

juce::StringArray PageInfo::Tree::getGroups(const String& page) const
{
	jassert(hasLayout);

	StringArray sa;

	for (const auto& p : pageTree)
	{
		if (p.first == page)
		{
			for (auto& g : p.second)
			{
				sa.addIfNotAlreadyThere(g.first);
			}
		}
	}

	return sa;
}

const scriptnode::ParameterDataList& PageInfo::Tree::getList(const String& page, const String& group) const
{
	if (!hasLayout)
		return flatList;

	for (const auto& p : pageTree)
	{
		if (page == p.first)
		{
			for (const auto& g : p.second)
			{
				if (group == g.first)
					return g.second;
			}
		}
	}

	jassertfalse;
	return {};
}

scriptnode::ParameterDataList& PageInfo::Tree::getBucket(const String& page, const String& group)
{
	for (auto& p : pageTree)
	{
		if (page == p.first)
		{
			for (auto& g : p.second)
			{
				if (group == g.first)
					return g.second;
			}

			p.second.push_back({ group, {} });
			return p.second.back().second;
		}
	}

	std::vector<std::pair<String, ParameterDataList>> newPage;
	newPage.push_back({ group, {} });
	pageTree.push_back({ page, newPage });
	return pageTree.back().second.back().second;
}

PageInfo::PageInfo(InputStream& mis)
{
	auto m = mis.readByte();

	if (m & HasPageMarker)
		page = mis.readString();

	if (m & HasGroupMarker)
		group = mis.readString();
}

PageInfo::PageInfo(const parameter::pod& info) :
	page(info.pageName),
	group(info.subGroupName)
{

}

void PageInfo::writeToStream(OutputStream& mos) const
{
	char m = 0;

	if (isPage())
		m |= HasPageMarker;

	if (isGroup())
		m |= HasGroupMarker;

	mos.writeByte(m);

	if (isPage())
		mos.writeString(page);
	if (isGroup())
		mos.writeString(group);
}

std::unique_ptr<scriptnode::PageInfo::Tree> PageInfo::createPageTree(const ValueTree& parameterTree)
{
	jassert(parameterTree.getType() == PropertyIds::Parameters);

	ParameterDataList list;

	for (const auto& p : parameterTree)
	{
		parameter::data d;
		d.info = parameter::pod(p);
		list.add(std::move(d));
	}

	return std::make_unique<Tree>(list);
}

bool PageInfo::hasPageData(const ValueTree& pTree)
{
	jassert(pTree.getType() == PropertyIds::Parameters);

	for (const auto& p : pTree)
	{
		if (p[PropertyIds::Page].toString().isNotEmpty())
			return true;
		if (p[PropertyIds::SubGroup].toString().isNotEmpty())
			return true;
	}

	return false;
}

}
