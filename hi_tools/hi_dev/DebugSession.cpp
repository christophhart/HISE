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

namespace hise { using namespace juce;

DebugSession::DebugSession(PooledUIUpdater* updater):
	PooledUIUpdater::SimpleTimer(updater, true)
{
	setEnableProfiling("Processing profile data");
}

DebugSession::~DebugSession()
{
	sessions.clear();
	currentMultithreadSession = nullptr;
	combinedRuns.clear();
}

ApiProviderBase* DebugSession::getProviderBase()
{
	return isActive() ? this : nullptr;
}

void DebugSession::getColourAndLetterForType(int type, Colour& colour, char& letter)
{
	switch(type)
	{
	case 3:
		letter = 'D';
		colour = cppgen::Helpers::getColourForType(Types::ID::Double);
		break;
	case 2:
		letter = 'G';
		colour = cppgen::Helpers::getColourForType(Types::ID::Block);
		break;
	case 1:
		letter = 'C';
		colour = cppgen::Helpers::getColourForType(Types::ID::Integer);
		break;
	}
}

DebugInformationBase::Ptr DebugSession::getDebugInformation(int index)
{
	if(isPositiveAndBelow(index, flushedSessions.size()))
		return flushedSessions[index];

	index -= flushedSessions.size();

	if(isPositiveAndBelow(index, sessions.size()))
		return sessions[index];

	return nullptr;
}

bool DebugSession::addDataItem(DataItem::Ptr newItem)
{
	bool found = false;

	if(auto s = getCurrentSession())
	{
		

		for(auto& vg: s->valueGroup)
		{
			if(newItem->id == vg->location.first &&
				newItem->lineNumber == vg->location.second)
			{
				vg->dataItems.add(newItem);
				newItem->idx = vg->dataItems.size() - 1;
				found = true;
				break;
			}
		}

		if(!found)
		{
			auto vg = new Session::ValueGroup();
			vg->p = s->p;
			vg->label = newItem->label;
			vg->location = { newItem->id, newItem->lineNumber };
			vg->dataItems.add(newItem);
			newItem->idx = 0;
			s->valueGroup.add(vg);
			found = true;
		}
	}

	if(isRecordingMultithread())
	{
		auto t = getCurrentThreadIdentifier();

		if(t.threadId != nullptr)
		{
			recordedDataItems.add(newItem);
			auto idx = recordedDataItems.size() - 1;

			for(auto& mv: multithreadedEventQueue)
			{
				if(mv->thread == t)
					mv->queue.push({ Event::EventType::DataItemEvent, recordingStart, idx, true });
			}
		}
	}


	return found;
}

void DebugSession::clearData(ApiProviderBase::Holder* p)
{
	MessageManagerLock m;
	for(int i = 0; i < sessions.size(); i++)
	{
		if(sessions[i]->p == p)
			sessions.remove(i--);
	}

	if(p == this)
	{
		flushedRoots.clear();
		currentMultithreadSession = nullptr;
		combinedRuns.clear();
	}
}

DebugInformationBase* DebugSession::startSession(ApiProviderBase::Holder* p, const String& sessionId)
{
	if(sessionId.isEmpty())
		sessions.removeLast();

	for(auto s: sessions)
	{
		if(s->id == sessionId)
		{
			s->p = p;
			s->valueGroup.clear();
			return s;
		}
	}

	auto s = new Session();
	s->id = sessionId;
	s->p = p;

	sessions.add(s);
	return sessions.getLast().get();
}

void DebugSession::popSession()
{
	if(auto s = sessions.removeAndReturn(sessions.size() - 1))
		flushedSessions.add(s);
}

bool DebugSession::isActive() const
{
	return !sessions.isEmpty();
}

void DebugSession::SerialisedData::setProperty(const Identifier& id, const var& newValue, void* unused)
{
	values.set(id, newValue);
}

void DebugSession::SerialisedData::addChild(SerialisedData::Ptr d, int unused1, void* unused2)
{
	children.add(d);
	numTotalItems += d->numTotalItems;
}

DebugSession::SerialisedData::Ptr DebugSession::SerialisedData::fromStream(InputStream& input)
{
	auto t = Identifier(input.readString());
	auto s = create(t);

	auto numProperties = input.readCompressedInt();

	for(int i = 0; i < numProperties; i++)
	{
		auto id = Identifier(input.readString());
		auto value = var::readFromStream(input);
		s->values.set(id, value);
	}

	auto numChildren = input.readCompressedInt();

	for(int i = 0; i < numChildren; i++)
		s->children.add(fromStream(input));

	return s;
}

bool DebugSession::SerialisedData::writeToStream(OutputStream& output)
{
	auto ok = output.writeString(getType().toString());
	ok &= output.writeCompressedInt(values.size());

	for(const auto& nv: values)
	{
		ok &= output.writeString(nv.name.toString());
		nv.value.writeToStream(output);
	}

	ok &= output.writeCompressedInt(children.size());

	for(auto c: children)
		ok &= c->writeToStream(output);

	return ok;
}

std::string DebugSession::SerialisedData::toString(int intend) const
{
	std::string s;

	for(int i = 0; i < intend; i++)
		s += " ";

	for(const auto& v: values)
	{
		s += v.name.toString().toStdString();
		s += ":" + v.value.toString().toStdString();
		s += ", ";
	}

	s += "\n";

	for(const auto& c: children)
	{
		s += c->toString(intend + 1);
	}

	return s;
}

int DebugSession::SerialisedData::getNumChildren() const
{ return children.size(); }

DebugSession::SerialisedData::Ptr DebugSession::SerialisedData::getChildWithName(const Identifier& id) const
{
	for(const auto& c: *this)
	{
		if(c->type == id)
			return c;
	}

	return nullptr;
}

Component* DebugSession::ItemBase::createPopupComponent(const MouseEvent& e, Component* componentToNotify)
{
	return createComponent(e.mods.isAnyModifierKeyDown());
	
}

Component* DebugSession::ItemBase::createComponent(bool forceJSON)
{
	auto data = getVariantCopy();

	if(!forceJSON && RectViewer::wantsRectangleViewer(data))
		return new RectViewer(getTextForName(), data);

	if(!forceJSON && BufferViewer::isArrayOrBuffer(getVariantCopy()))
		return new BufferViewer(this, p);

	return new JSONViewer(data);
}

String DebugSession::DataItem::getTextForName() const
{
	String s;

	if(!label.isEmpty())
		s << label;
	else
		s << id.toString() << ":" + String(lineNumber);

	s << "[" << String(idx) << "]";

	return s;
}

DebugSession::SerialisedData::Ptr DebugSession::DataItem::toSerialisedData() const
{
	auto v = SerialisedData::create(ProfileIds::dataItem);

	v->setProperty(ProfileIds::sourceIndex, idx);
	v->setProperty(ProfileIds::dlocName, id.toString());
	v->setProperty(ProfileIds::dlocPos, lineNumber);
	v->setProperty(ProfileIds::label, label);
	v->setProperty(ProfileIds::value, data);

	return v;
}

DebugSession::DataItem::Ptr DebugSession::DataItem::fromSerialisedData(SerialisedData::Ptr v_)
{
	auto& v = *v_;
	auto ni = new DataItem();

	ni->idx = (int)v[ProfileIds::sourceIndex];
	ni->data = v[ProfileIds::value];
	ni->lineNumber = (int)v[ProfileIds::dlocPos];
	ni->label = v[ProfileIds::label].toString();
	ni->id = v[ProfileIds::dlocName].toString();

	return ni;
}


String DebugSession::ProfileDataSource::ViewComponents::Helpers::getDuration(float w, TimeDomain domain,
                                                                             double contextValue)
{
	String m;

	if(domain == TimeDomain::Absolute)
	{
		if(w < 0.005)
		{
			
			m << String(w * 1000.0, 3) << " " << String::charToString(181) << "s";
		}
		else
		{
			m << String(w, 3) << " ms";
		}
	}
	if(domain == TimeDomain::Relative)
	{
		w /= contextValue; // contextValue is the length of the root item
		m << String(w * 100.0, 1) << "%";
	}
	if(domain == TimeDomain::FPS60)
	{
		w *= 0.001f;
		w = jmax(0.0001f, w);
		auto frameTime = 1.0f / w;
		m << String(frameTime, 1) << " FPS";
	}
	if(domain == TimeDomain::CpuUsage)
	{
		// contextValue is the length of a audio buffer in milliseconds
		w /= contextValue;
		m << String(w * 100.0, 2) << "%";
	}

	return m;
}

void DebugSession::ProfileDataSource::ViewComponents::Helpers::drawLabel(Graphics& g,
                                                                         const std::pair<Rectangle<float>, String>& l)
{
	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colour(0xFF1D1D1D));
	g.fillRect(l.first);
	g.setColour(Colours::white.withAlpha(0.6f));
	g.drawText(l.second, l.first, Justification::centred);
}

void DebugSession::ProfileDataSource::Profiler::startProfiling(ApiProviderBase::Holder* holder)
{
	if(*this && holder != nullptr)
	{
		session = holder->getDebugSession();
		data->holder = holder;
		session->startScopedProfile(data);
	}
	else
		data = nullptr;
}

void DebugSession::ProfileDataSource::Profiler::stopProfiling(ApiProviderBase::Holder* holder)
{
	if(*this)
	{
		if(holder != nullptr)
			session = holder->getDebugSession();

		jassert(session != nullptr);

		session->pushProfileInfo(data);
	}
}

DebugSession::ProfileDataSource::ScopedProfiler::ScopedProfiler(ProfileDataSource::Ptr statementData,
                                                                ApiProviderBase::Holder* holder):
	Profiler(statementData)
{
	startProfiling(holder);
}

DebugSession::ProfileDataSource::ScopedProfiler::~ScopedProfiler()
{
	stopProfiling();
}






DebugSession::ProfileDataSource::RecordingSession::RecordingSession(DebugSession& session_, ThreadIdentifier::Type t):
	session(session_),
	threadData(new ProfileDataSource()),
	currentRecorder(*this),
	type(t)
{
	threadData->name = ThreadIdentifier::getThreadName(t);
	threadData->threadRoot = t;
	threadData->colour = Colour(0xDD555555);
	threadData->sourceType = SourceType::RecordingSession;
}

DebugSession::ProfileDataSource::RecordingSession::~RecordingSession()
{
	ScopedLock sl(session.recordingLock);

	session.availableRecordingSessions.removeAllInstancesOf(this);

	for(auto t: session.multithreadedEventQueue)
	{
		if(t->thread == getThread())
		{
			session.multithreadedEventQueue.removeObject(t);
			break;
		}
	}
}

void DebugSession::ProfileDataSource::RecordingSession::setRecordingState(RecordingState state,
	ApiProviderBase::Holder* h)
{
	DBG_P(ThreadIdentifier::getThreadName(type) + ".rec = " + String((int)state));
	jassert(state == RecordingState::Armed || state == RecordingState::WaitingForStop);
	holder = h;
	nextState.store(state);
}

void DebugSession::ProfileDataSource::RecordingSession::checkRecording()
{
	if(!enabled)
		return;

	ThreadIdentifier c;

	if(recordedThread.type == ThreadIdentifier::Type::UIThread)
		c = session.uiThread;
	else
		c = ThreadIdentifier::getCurrent();

	

	if(recordedThread != c)
	{
		jassertfalse;
		enabled = false;
		return;
	}

	if(nextState == RecordingState::Armed)
	{
		DBG_P(ThreadIdentifier::getThreadName(type) + ".start()");
		//currentRecorder.startProfiling(holder.get());
		nextState.store(RecordingState::Recording);
	}
		

	if(nextState == RecordingState::WaitingForStop)
	{
		DBG_P(ThreadIdentifier::getThreadName(type) + ".stop()");
		//currentRecorder.stopProfiling(holder.get());
		nextState.store(RecordingState::Idle);
	}
}



bool DebugSession::ProfileDataSource::RecordingSession::isDataSource( DebugInformationBase::Ptr firstChildOfCombinedRun) const
{
	if(firstChildOfCombinedRun == nullptr)
		return false;

	if(auto typed = dynamic_cast<ProfileInfo*>(firstChildOfCombinedRun.get()))
		return typed->data.source == threadData;

	return false;
}

void DebugSession::ProfileDataSource::RecordingSession::initIfEmpty(ThreadIdentifier t)
{
	if(recordedThread.threadId == nullptr && t.threadId != nullptr)
	{
		ScopedLock sl(session.recordingLock);

		recordedThread = t.withType(type);
		session.availableRecordingSessions.add(this);
		session.multithreadedEventQueue.add(new Event::Queue(recordedThread));
	}
}

DebugSession::ProfileDataSource::RecordingSession::Recorder::Recorder(RecordingSession& parent_):
	Profiler(parent_.threadData),
	parent(parent_)
{}


String DebugSession::ProfileDataSource::ProfileInfo::getTextForValue() const
{
	String t = data.source != nullptr ? data.source->name : "";

	if(data.loopIndex != -1)
		t << "[" << String(data.loopIndex) << "]";

	return t;
}

void DebugSession::ProfileDataSource::ProfileInfo::gotoLocation(Component* clickedComponent)
{
	if(data.source != nullptr && d.gotoLocation)
	{
		d.gotoLocation(clickedComponent, data.source->sourceType, data.source->locationString);
	}
}

bool DebugSession::ProfileDataSource::ProfileInfo::canBeOptimized() const
{
	if(data.source->durationThreshold != 0.0 && data.delta < data.source->durationThreshold && children.isEmpty())
	{
		return true;
	}

	return false;
}

DebugSession::SerialisedData::Ptr DebugSession::ProfileDataSource::ProfileInfo::toSerialisedData(
	ProfileDataSource::List& existingSources, DataItem::List& existingData) const
{
	auto r = ProfileInfoBase::toSerialisedData(existingSources, existingData);

	auto idx = existingSources.indexOf(data.source);

	if(idx == -1)
	{
		existingSources.add(data.source);
		idx = existingSources.size()-1;
	}

	Array<var> dataItems;

	for(auto d: data.attachedData)
	{
		existingData.add(d);
		dataItems.add(existingData.size()-1);
	}

	if(!dataItems.isEmpty())
		r->setProperty(ProfileIds::dataList, var(dataItems));

	r->setProperty(ProfileIds::sourceIndex, idx);
	r->setProperty(ProfileIds::length, data.delta);
	r->setProperty(ProfileIds::start, data.start);
	r->setProperty(ProfileIds::loopCounter, (int64)data.loopIndex);
	r->setProperty(ProfileIds::runIndex, (int64)data.runIndex);
	r->setProperty(ProfileIds::thread, (int)data.threadType);
	r->setProperty(ProfileIds::trackSource, data.trackSource);
	r->setProperty(ProfileIds::trackTarget, data.trackTarget);
				
	return r;
}

void DebugSession::ProfileDataSource::ProfileInfo::fromSerialisedData(SerialisedData::Ptr v_,
	const ProfileDataSource::List& existingSources, const DataItem::List& existingList, ProfileDataSource::Ptr parent,
	ApiProviderBase::Holder* h)
{
	auto& v = *v_;

	auto idx = (int)v[ProfileIds::sourceIndex];
	data.source = existingSources[idx];
	jassert(data.source != nullptr);

	auto l = v[ProfileIds::dataList];

	if(l.isArray())
	{
		for(auto& v: *l.getArray())
		{
			auto dataIndex = (int)v;

			if(isPositiveAndBelow(dataIndex, existingList.size()))
				data.attachedData.add(existingList[dataIndex]);
			else
			{
				// Can't find the item...
				jassertfalse;
			}
		}
	}

	data.parent = parent;
	data.start = v[ProfileIds::start];
	data.delta = v[ProfileIds::length];
	data.runIndex = (int64)v[ProfileIds::runIndex];
	data.loopIndex = (int64)v[ProfileIds::loopCounter];
	data.threadType = (ThreadIdentifier::Type)(int)v[ProfileIds::thread];
	data.trackSource = (int)v[ProfileIds::trackSource];
	data.trackTarget = (int)v[ProfileIds::trackTarget];
	data.holder = h;

	ProfileInfoBase::fromSerialisedData(v_, existingSources, existingList, parent, h);
}


void DebugSession::ProfileDataSource::ProfileInfo::generateHeatMap(std::map<int, double>& heatmap)
{
	auto r = data.source->lineRange;

	if(!r.isEmpty())
	{
		auto v = data.delta / (double)r.getLength();

		for(int i = r.getStart(); i < r.getEnd(); i++)
			heatmap[i] = jmax(v, heatmap[i]);
	}

	for(auto c: children)
		dynamic_cast<ProfileInfo*>(c)->generateHeatMap(heatmap);
	
}

DebugSession::ProfileDataSource::CombinedRoot::CombinedRoot(DebugSession& d_, ThreadIdentifier::Type t):
	ProfileInfoBase(d_),
	threadType(t)
{
	jassert(threadType != ThreadIdentifier::Type::Undefined);
}

DebugSession::SerialisedData::Ptr DebugSession::ProfileDataSource::CombinedRoot::toSerialisedData(
	ProfileDataSource::List& existingSources, DataItem::List& existingData) const
{
	auto v = ProfileInfoBase::toSerialisedData(existingSources, existingData);
	v->setProperty(ProfileIds::thread, (int)threadType);
	return v;
}

void DebugSession::ProfileDataSource::CombinedRoot::fromSerialisedData(SerialisedData::Ptr v_,
                                                                       const ProfileDataSource::List& existingSources, const DataItem::List& existingData, ProfileDataSource::Ptr parent,
                                                                       ApiProviderBase::Holder* h)
{
	ProfileInfoBase::fromSerialisedData(v_, existingSources, existingData, parent, h);
	auto& v = *v_;
	threadType = (ThreadIdentifier::Type)(int)v[ProfileIds::thread];
}

void DebugSession::ProfileDataSource::MultiThreadSession::fromSerialisedData(SerialisedData::Ptr v_,
	const ProfileDataSource::List& existingSources, const DataItem::List& existingData, ProfileDataSource::Ptr parent,
	ApiProviderBase::Holder* h)
{
	auto& v = *v_;
	jassert(v.getType() == getProfileId());
	jassert(h != nullptr);
	jassert(parent == nullptr);

	for(auto c: *v.getChildWithName(ProfileIds::children))
	{
		auto cr = new ProfileInfo(d);
		cr->fromSerialisedData(*c, existingSources, existingData, nullptr, h);
		addChild(cr);
	}
}

DebugSession::ProfileDataSource::ProfileDataSource()
{
	colour = Colours::red.withHue(Random::getSystemRandom().nextFloat()).withSaturation(0.4f).withBrightness(0.5f);
}

DebugSession::ProfileDataSource::Ptr DebugSession::ProfileDataSource::fromSerialisedData(SerialisedData::Ptr d_)
{
	auto& d = *d_;
	jassert(d.getType() == ProfileIds::profile);

	auto ptr = new ProfileDataSource();

	ptr->name = d[ProfileIds::name].toString();
	ptr->lineRange = {(int)d[ProfileIds::lineStart], (int)d[ProfileIds::lineEnd]};
	ptr->preferredDomain = (TimeDomain)(int)d[ProfileIds::domain];
	ptr->locationString = d[ProfileIds::dlocName].toString();
	ptr->onMessageFlush = {};
	ptr->sourceType = (SourceType)(int)d[ProfileIds::sourceType];
	ptr->colour = Colour((uint32)(int64)d[ProfileIds::colour]);
	ptr->threadRoot = (ThreadIdentifier::Type)(int)d[ProfileIds::thread];
	return ptr;
}

void DebugSession::ProfileDataSource::writeIntoSerialisedData(SerialisedData::Ptr d_) const
{
	auto& d = *d_;
	jassert(d.getType() == ProfileIds::profile);

	d.setProperty(ProfileIds::name, name, nullptr);
	d.setProperty(ProfileIds::lineStart, lineRange.getStart(), nullptr);
	d.setProperty(ProfileIds::lineEnd, lineRange.getEnd(), nullptr);
	d.setProperty(ProfileIds::domain, (int)preferredDomain, nullptr);
	d.setProperty(ProfileIds::dlocName, locationString, nullptr);
	d.setProperty(ProfileIds::sourceType, (int)sourceType, nullptr);
	d.setProperty(ProfileIds::colour, (int64)colour.getARGB(), nullptr);
	d.setProperty(ProfileIds::thread, (int)threadRoot, nullptr);
}

DebugSession::SerialisedData::Ptr DebugSession::ProfileDataSource::ProfileInfoBase::toSerialisedData(
	ProfileDataSource::List& existingSources, DataItem::List& existingData) const
{
	auto r = new SerialisedData(getProfileId());
	auto cr = new SerialisedData(ProfileIds::children);

	for(auto c: children)
		cr->addChild(dynamic_cast<ProfileInfo*>(c)->toSerialisedData(existingSources, existingData), -1, nullptr);

	r->addChild(cr, -1, nullptr);

	return r;
}

String DebugSession::ProfileDataSource::ProfileInfoBase::toBase64() const
{
	String b64;
	b64 << "HiseProfile";

	MemoryOutputStream mos;

	ProfileDataSource::List existingSources;
	DataItem::List existingData;
	SerialisedData::Ptr sd = toSerialisedData(existingSources, existingData);

	auto sourceList = SerialisedData::create(ProfileIds::sourceList);
	auto dataList = SerialisedData::create(ProfileIds::dataList);

	for(auto d: existingData)
	{
		dataList->addChild(d->toSerialisedData());
	}

	for(auto s: existingSources)
	{
		auto p = SerialisedData::create(ProfileIds::profile);
		s->writeIntoSerialisedData(p);
		sourceList->addChild(p);
	}

	dataList->writeToStream(mos);
	sourceList->writeToStream(mos);
	sd->writeToStream(mos);
	mos.flush();

	auto mb = mos.getMemoryBlock();
	MemoryBlock compressed;

	zstd::ZDefaultCompressor comp;
	comp.compress(mb, compressed);

	b64 << compressed.toBase64Encoding();
	return b64;
}

DebugSession::ProfileDataSource::ProfileInfoBase::Ptr DebugSession::ProfileDataSource::ProfileInfoBase::fromBase64(
	const String& b64, DebugSession& h)
{
	auto c = b64.substring(String("HiseProfile").length());
	MemoryBlock compressed;
	if(compressed.fromBase64Encoding(c))
	{
		zstd::ZDefaultCompressor comp;

		MemoryBlock mb;
		if(comp.expand(compressed, mb))
		{
			MemoryInputStream mis(mb.getData(), mb.getSize(), false);
			auto dataList = SerialisedData::fromStream(mis);
			auto sourceList = SerialisedData::fromStream(mis);

			ProfileDataSource::List existingSources;
			DataItem::List existingData;

			for(auto c: *dataList)
				existingData.add(DataItem::fromSerialisedData(c));

			for(auto c: *sourceList)
				existingSources.add(ProfileDataSource::fromSerialisedData(c));

			auto tree = SerialisedData::fromStream(mis);

			ProfileInfoBase::Ptr ni;

			if(tree->getType() == ProfileIds::multithread)
				ni = new MultiThreadSession(h);
			else if (tree->getType() == ProfileIds::root)
			{
				auto threadType = (ThreadIdentifier::Type)(int)(*(tree))[ProfileIds::thread];
				ni = new CombinedRoot(h, threadType);
			}
			else
				ni = new ProfileInfo(h);
						
			ni->fromSerialisedData(tree, existingSources, existingData, nullptr, &h);
			return ni;
		}
	}

	return nullptr;
}

DebugSession::ProfileDataSource::ProfileInfoBase::Ptr DebugSession::ProfileDataSource::ProfileInfoBase::combineRuns(
	DebugSession& session, Ptr rootInfo, Ptr infoItem)
{
	ProfileInfoBase::Ptr clone;

	if(infoItem->getProfileId() == ProfileIds::profile)
	{
		auto di = dynamic_cast<ProfileInfo*>(infoItem.get());
					
		auto ni = new ProfileDataSource::CombinedRoot(session, di->data.threadType);
		ni->sourceForRuns = di->getSource();
		clone = ni;

		ProfileDataSource::List existingSources;
		DataItem::List existingData;

		rootInfo->callRecursive([&](DebugInformationBase& b)
		{
			if(auto typed = dynamic_cast<ProfileInfo*>(&b))
			{
				if(typed->getSource() == di->getSource())
				{
					auto cl = new ProfileInfo(session);
					auto d = typed->toSerialisedData(existingSources, existingData);
					cl->fromSerialisedData(d, existingSources, existingData, nullptr, typed->data.holder);
					clone->addChild(cl);
				}
			}

			return false;
		});
	}

	return clone;
}

DebugSession::ProfileDataSource::ProfileInfoBase::Ptr DebugSession::ProfileDataSource::ProfileInfoBase::
trimToCurrentRange(DebugSession& session, Range<double> currentRange, ProfileInfoBase::Ptr rootInfo)
{
	//auto currentRange = getNormalisedZoomRange();
	//auto rootInfo = originalRoot->infoItem.get();

	//currentRange = { currentRange.getStart() * totalLength, currentRange.getEnd() * totalLength };

	auto firstInfo = dynamic_cast<ProfileInfo*>(rootInfo.get());

	if(firstInfo == nullptr)
		firstInfo = dynamic_cast<ProfileInfo*>(rootInfo->children[0].get());


	if(firstInfo != nullptr)
		currentRange += firstInfo->data.start;

	ProfileInfoBase::Ptr clone;

	if(rootInfo->getProfileId() == ProfileIds::multithread)
	{
		ProfileDataSource::List existingSources;
		DataItem::List existingData;
		auto d = rootInfo->toSerialisedData(existingSources, existingData);

		clone = new MultiThreadSession(session);
		clone->fromSerialisedData(*d, existingSources, existingData, nullptr, &session);

		clone->callRecursive([currentRange](DebugInformationBase& b)
		{
			if(auto di = dynamic_cast<ProfileInfo*>(&b))
			{
				Range<double> dr(di->data.start, di->data.start + di->data.delta);

				auto clipped = currentRange.getIntersectionWith(dr);

				if(clipped.isEmpty())
				{
					di->data.delta = 0.0;
					di->children.clear();
				}
				else
				{
					di->data.start = clipped.getStart();
					di->data.delta = clipped.getLength();
				}
			}

			return false;
		});

		clone->callRecursive([currentRange](DebugInformationBase& b)
		{
			auto db = dynamic_cast<ProfileInfoBase*>(&b);

			for(int i = 0; i < db->children.size(); i++)
			{
				if(auto di = dynamic_cast<ProfileInfo*>(db->children[i].get()))
				{
					if(di->data.delta == 0.0)
					{
						db->children.remove(i--);
					}
				}
			}

			return false;
		});

		ProfileDataSource::List src;
		DataItem::List data;
		auto s = clone->toSerialisedData(src, data);
		DBG(s->toString(0));
	}

	return clone;
}

void DebugSession::ProfileDataSource::ProfileInfoBase::fromSerialisedData(SerialisedData::Ptr v_,
	const ProfileDataSource::List& existingSources, const DataItem::List& existingData, ProfileDataSource::Ptr parent,
	ApiProviderBase::Holder* h)
{
	auto& v = *v_;
	jassert(h != nullptr);
	jassert(v.getType() == getProfileId());

	children.clear();

	for(auto c: *v.getChildWithName(ProfileIds::children))
	{
		auto ptr = new ProfileInfo(d);
		ptr->fromSerialisedData(*c, existingSources, existingData, getSource(), h);
		addChild(ptr);
	}
}

Component* DebugSession::ProfileDataSource::ProfileInfoBase::createPopupComponent(const MouseEvent& e,
                                                                                  Component* componentToNotify)
{
	return d.createPopupViewer(this);
	//return createComponent();
	
}

Component* DebugSession::ProfileDataSource::ProfileInfoBase::createComponent()
{
	return new ViewComponents::Viewer(this, d, true);
}

void DebugSession::ProfileDataSource::ProfileInfoBase::addChild(ProfileInfoBase::Ptr p)
{
	jassert(p.get() != this);

	if(children.contains(p))
		return;

	children.add(p);
	p->parent = this;
}

DebugSession::ProfileDataSource::ProfileInfoBase* DebugSession::ProfileDataSource::ProfileInfoBase::getRoot()
{
	if(auto p = dynamic_cast<ProfileInfoBase*>(parent.get()))
		return p->getRoot();

	return this;
}

void DebugSession::ProfileDataSource::ProfileInfoBase::optimize()
{
	for(int i = 0; i < getNumChildElements(); i++)
	{
		auto c = getChildElement(i);

		if(dynamic_cast<ProfileInfoBase*>(c.get())->canBeOptimized())
		{
			children.remove(i--);
			continue;
		}
	}

	for(auto c: children)
		c->optimize();
}

void DebugSession::startScopedProfile(ProfileDataSource::Ptr newParent)
{
	auto t = getCurrentThreadIdentifier();

	if(t.threadId == nullptr)
		return;

	jassert(newParent != nullptr);

	bool found = false;

	for(auto& q: multithreadedEventQueue)
	{
		if(q->thread == t)
		{
			q->queue.push({newParent, recordingStart, true});
			found = true;
			break;
		}
	}

	if(!found)
	{
		DBG("WARNING: unknown thread for profiling");
	}
}

void DebugSession::pushProfileInfo(ProfileDataSource::Ptr dataToPop)
{
	auto t = getCurrentThreadIdentifier();

	if(t.threadId == nullptr)
		return;

	for(auto q: multithreadedEventQueue)
	{
		if(q->thread == t)
			q->queue.push({dataToPop, recordingStart, false});
	}
}

DebugInformationBase::Ptr DebugSession::getLastProfileRoot(ThreadIdentifier::Type t) const
{
	for(int i = combinedRuns.size() - 1; i >= 0; i--)
	{
		if(combinedRuns[i]->threadType == t)
			return combinedRuns[i];
	}

	return combinedRuns.getLast();
}

void DebugSession::startRecording(double milliSeconds, ApiProviderBase::Holder* h)
{
	if(nextState.load() == RecordingState::Idle)
	{
		nextState.store(RecordingState::Armed);
		recordingDelta = currentOptions.millisecondsToRecord;// milliSeconds;

		if(currentOptions.trigger != TriggerType::Manual)
			recordingDelta = 20000.0;

		recordHolder = h;

		if(MessageManager::getInstanceWithoutCreating()->currentThreadHasLockedMessageManager())
		{
			timerCallback();
		}
	}
}


void DebugSession::stopRecording()
{
	recordingDelta = 0.0;

	if(MessageManager::getInstanceWithoutCreating()->currentThreadHasLockedMessageManager())
	{
		timerCallback();
	}
}

void DebugSession::initUIThread()
{
	if(uiThread.type == ThreadIdentifier::Type::Undefined)
	{
		uiThread = ThreadIdentifier::getCurrent().withType(ThreadIdentifier::Type::UIThread);
	}
}

Component* DebugSession::createPopupViewer(const DebugInformationBase::Ptr ptr)
{
	return new ProfileDataSource::ViewComponents::SingleThreadPopup(*this, ptr);
		
}

void DebugSession::addAudioThread(ThreadIdentifier t)
{
	registeredAudioThreads.insert(t);
}

void DebugSession::checkAudioThreadRecorders()
{
	for(auto r: audioThreadRecorders)
		r->checkRecording();
}

void DebugSession::checkMouseClickProfiler(bool isDown)
{
	if(currentOptions.trigger == TriggerType::MouseClick)
	{
		if(isDown)
			startRecording(-1.0, this);
		else
		{
			recordingDelta = 100.0;
		}
	}
}

int DebugSession::openTrackEvent()
{
	auto t = getCurrentThreadIdentifier();
	if(t.threadId == nullptr)
		return -1;

	auto thisId = ++trackCounter;

	for(auto& mv: multithreadedEventQueue)
	{
		if(mv->thread == t)
			mv->queue.push({ Event::EventType::TrackEvent, recordingStart, thisId, true });
	}

	return thisId;
}

void DebugSession::closeTrackEvent(int id)
{
	auto t = getCurrentThreadIdentifier();

	if(t.threadId == nullptr)
		return;

	for(auto& mv: multithreadedEventQueue)
	{
		if(mv->thread == t)
			mv->queue.push({ Event::EventType::TrackEvent, recordingStart, id, false });
	}
}

void DebugSession::setGotoCallback(const GotoLocationCallback& f)
{
	gotoLocation = f;
}

DebugSession::ThreadIdentifier DebugSession::getCurrentThreadIdentifier() const
{
	if(uiThread.type == ThreadIdentifier::Type::Undefined)
		return {};

	auto t = ThreadIdentifier::getCurrent();

	if(t == timerThread)
		t = uiThread;

	return t;
}


DebugSession::Session* DebugSession::getCurrentSession()
{
	return sessions.getLast().get();
}

void DebugSession::initAudioThreadAndMessageThreadRecorders()
{
	if(messageThreadRecorder == nullptr)
	{
		if(uiThread.type == ThreadIdentifier::Type::UIThread)
		{
			timerThread = ThreadIdentifier::getCurrent();

			jassert(MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread());
			messageThreadRecorder = new ProfileDataSource::RecordingSession(*this, ThreadIdentifier::Type::UIThread);
			messageThreadRecorder->initIfEmpty(uiThread);
		}
		else
		{
			return;
		}
	}

	if(registeredAudioThreads.size() > audioThreadRecorders.size())
	{
		for(auto t: registeredAudioThreads)
		{
			bool found = false;

			for(auto r: audioThreadRecorders)
			{
				if(r->getThread() == t)
				{
					found = true;
					break;
				}
			}

			if(!found)
			{
				audioThreadRecorders.add(new ProfileDataSource::RecordingSession(*this, ThreadIdentifier::Type::AudioThread));
				audioThreadRecorders.getLast()->initIfEmpty(t);
			}
		}
	}
}

void DebugSession::timerCallback()
{
	initAudioThreadAndMessageThreadRecorders();

	processEventQueue();
	
	if(nextState == RecordingState::Armed)
	{
		for(auto& tv: multithreadedEventQueue)
		{
			tv->stack.clear();
		}

		if(isEventQueueEmpty())
		{
			ScopedLock sl(recordingLock);
			
			for(auto& t: multithreadedEventQueue)
				t->stack.clear();

			syncRecordingBroadcaster.sendMessage(sendNotificationSync, true);
			flushedRoots.clear();
			combinedRuns.clear();
			recordedDataItems.clear();
			currentMultithreadSession = new ProfileDataSource::MultiThreadSession(*this);

			for(auto av: availableRecordingSessions)
			{
				av->setRecordingState(nextState.load(), recordHolder);
			}

			processEventQueue();
			nextState.store(RecordingState::WaitingForRecording);
			DBG_P("Debug.rec = " + String((int)nextState.load()));
		}
	}

	if(nextState == RecordingState::WaitingForRecording)
	{
		ScopedLock sl(recordingLock);

		auto isRecording = true;

		for(auto av: availableRecordingSessions)
		{
			auto p = av->getDataSource();
			p->holder = this;

			bool found = false;

			for(auto& q: multithreadedEventQueue)
			{
				if(q->thread == av->getThread())
				{
					q->queue.push({p.get(), recordingStart, true});
					found = true;
					break;
				}
			}
			
			isRecording &= found;
		}

		jassert(isRecording);

		if(isRecording)
		{
			recordingStart = Time::getMillisecondCounterHiRes();
			nextState.store(RecordingState::Recording);
			DBG_P("Debug.rec = " + String((int)nextState.load()));
		}
	}

	if(nextState == RecordingState::Recording)
	{
		processEventQueue();
		
		auto t = Time::getMillisecondCounterHiRes();

		if(t > (recordingStart + recordingDelta))
		{
			
			nextState.store(RecordingState::WaitingForStop);
			DBG_P("Debug.rec = " + String((int)nextState.load()));

			ScopedLock sl(recordingLock);

			for(auto av: availableRecordingSessions)
			{
				av->setRecordingState(nextState.load(), recordHolder);
			}
		}
	}

	if(nextState == RecordingState::WaitingForStop)
	{
		ScopedLock sl(recordingLock);

		for(auto& q: multithreadedEventQueue)
		{
			for(int i = (int)q->stack.size() - 1; i >= 0; i--)
				q->queue.push({q->stack[i]->data.source.get(), recordingStart, false});
		}

		if(isEventQueueEmpty())
		{
			for(auto& r: flushedRoots)
			{
				auto tr = r.second->data.source->threadRoot;

				auto addThread = std::find(currentOptions.threadFilter.begin(), currentOptions.threadFilter.end(), tr) != currentOptions.threadFilter.end();

				if(!addThread)
					continue;

				if(tr != ThreadIdentifier::Type::Undefined && !r.second->children.isEmpty())
				{
					currentMultithreadSession->addChild(r.second);
				}
			}

			struct Sorter
			{
				static int compareElements(ProfileDataSource::ProfileInfoBase* p1, ProfileDataSource::ProfileInfoBase* p2)
				{
					auto t1 = dynamic_cast<ProfileDataSource::ProfileInfo*>(p1)->data.source->threadRoot;
					auto t2 = dynamic_cast<ProfileDataSource::ProfileInfo*>(p2)->data.source->threadRoot;

					if((int)t1 < (int)t2)
						return -1;
					if((int)t1 > (int)t2)
						return 1;

					return 0;
				}
			} sorter;

			currentMultithreadSession->children.sort(sorter);

			ProfileDataSource::ProfileInfoBase::Ptr s = currentMultithreadSession.get();

			syncRecordingBroadcaster.sendMessage(sendNotificationSync, false);
			recordingFlushBroadcaster.sendMessage(sendNotificationSync, s);
			clearData(this);
			nextState.store(RecordingState::Idle);
			DBG_P("Debug.rec = " + String((int)nextState.load()));
		}
	}
}

void DebugSession::processEventQueue()
{
	ScopedLock sl(recordingLock);

	if(messageThreadRecorder == nullptr)
		return;

	messageThreadRecorder->checkRecording();

	for(auto t: multithreadedEventQueue)
	{
		std::vector<Event> thisEvents;
		thisEvents.reserve(t->getNumEventsInQueue());

		t->queue.callForEveryElementInQueue([&](const Event& e)
		{
			thisEvents.push_back(e);
			return true;
		});

		for(const auto&e: thisEvents)
		{
			if(e.isValueEvent())
			{
				if(!t->stack.empty())
				{
					auto et = e.getEventType();

					if(et == Event::EventType::TrackEvent)
					{
						if(e.isPush())
							t->stack.back()->data.trackSource = e.getValue();
						else
							t->stack.back()->data.trackTarget = e.getValue();
					}
					else if (et == Event::EventType::DataItemEvent)
					{
						if(auto item = recordedDataItems[e.getValue()])
							t->stack.back()->data.attachedData.add(item);
					}
				}
			}
			else
			{
				if(e.isPush())
				{
					ProfileDataSource::Data newData;

					newData.parent = t->stack.empty() ? nullptr : t->stack.back()->getSource();
					newData.source = e.getData();
					newData.threadType = t->thread.type;

					if(newData.source->isLoop)
						newData.source->loopCounter = 0;

					if(newData.parent != nullptr && newData.parent->isLoop)
					{
						newData.parent->loopCounter++;
						newData.loopIndex = newData.parent->loopCounter - 1;
					}

					if(newData.loopIndex > MaxLoopCounter)
						continue;

					newData.start = e.getTime();
					newData.holder = newData.source->holder;

					auto ni = new ProfileDataSource::ProfileInfo(*this);
					ni->data = newData;

					if(!t->stack.empty())
						t->stack.back()->addChild(ni);

					t->stack.push_back(ni);
				}
				else
				{
					if(!t->stack.empty())
					{
						auto ni = t->stack.back();

						if(ni->getSource() != e.getData())
							continue;
						
						ni->data.delta = e.getTime() - ni->data.start;
						t->stack.pop_back();

                        if(ni->data.source->onMessageFlush)
                        {
                            ni->data.source->onMessageFlush(ni->data.holder, ni);
                        }
                        
						if(t->stack.empty())
						{
							

							ni->optimize();

							if(currentMultithreadSession != nullptr)
							{
								flushedRoots.push_back({t->thread, ni});
							}
							else
							{
								std::map<int, double> heatmap;
								ni->generateHeatMap(heatmap);

								if(!heatmap.empty())
								{
									auto comp = [] (const std::pair<int, double> & p1, const std::pair<int, double> & p2) { return p1.second < p2.second; };
									auto minValue = std::min_element(heatmap.begin(), heatmap.end(), comp)->second;
									auto maxValue = std::max_element(heatmap.begin(), heatmap.end(), comp)->second;

									for(auto & v: heatmap)
										v.second = (v.second - minValue) * 1.0 / (maxValue - minValue);

									ni->data.holder->heatmapManager.setHeatMap(ni, heatmap);
								}

								for(auto cr: combinedRuns)
								{
									if(cr->sourceForRuns == ni->getSource())
									{
										cr->addChild(ni);
										ni = nullptr;
										break;
									}
								}

								if(ni != nullptr)
								{
									auto nc = new ProfileDataSource::CombinedRoot(*this, t->thread.type);
									nc->sourceForRuns = ni->getSource();
									nc->addChild(ni);
									combinedRuns.add(nc);
								}
							}
						}
					}
				}
			}

			
		}
	}
}

bool DebugSession::isEventQueueEmpty()
{
	ScopedLock sl(recordingLock);

	processEventQueue();

	for(auto& tv: multithreadedEventQueue)
	{
		if(!tv->stack.empty())
			return false;
	}

	return true;
}

var DebugSession::Session::getVariantCopy() const
{
	DynamicObject::Ptr no = new DynamicObject();

	for(auto g: valueGroup)
	{
		no->setProperty(g->label, g->getVariantCopy());
	}
		
	
	return var(no.get());
}

int DebugSession::Session::ValueGroup::getNumChildElements() const
{ return dataItems.size(); }

String DebugSession::Session::ValueGroup::getTextForName() const
{
	if(label.isEmpty())
		return location.first + ":" + String(location.second);
	else
		return label;
}

String DebugSession::Session::ValueGroup::getCodeToInsert() const
{ return getTextForName(); }

DebugInformationBase::Ptr DebugSession::Session::ValueGroup::getChildElement(int index)
{
	return dataItems[index];
}

var DebugSession::Session::ValueGroup::getVariantCopy() const
{
	if(dataItems.size() == 1)
		return dataItems.getFirst()->data;

	auto sameLabels = true;

	for(int i = 1; i < dataItems.size(); i++)
		sameLabels &= dataItems[i]->label == dataItems[0]->label;

	if(sameLabels)
	{
		Array<var> data;

		for(auto d: dataItems)
			data.add(d->data);

		return var(data);
	}
	else
	{
		auto no = new DynamicObject();

		for(auto d: dataItems)
			no->setProperty(d->label, d->data);

		return var(no);
	}
}
} // namespace hise
