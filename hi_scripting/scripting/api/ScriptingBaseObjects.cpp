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

namespace hise { using namespace juce;

ScriptingObject::ScriptingObject(ProcessorWithScriptingContent *p) :
processor(p),
thisAsProcessor(dynamic_cast<Processor*>(p))
{
	jassert((thisAsProcessor != nullptr) == (processor != nullptr));
};

ProcessorWithScriptingContent *ScriptingObject::getScriptProcessor()
{
	return processor.get();
};

const ProcessorWithScriptingContent *ScriptingObject::getScriptProcessor() const
{
	return processor.get();
};


bool ScriptingObject::checkArguments(const String &callName, int numArguments, int expectedArgumentAmount)
{
	if (numArguments < expectedArgumentAmount)
	{
		String x;
		x << "Call to " << callName << " - Too few arguments: " << String(numArguments) << ", (Expected: " << String(expectedArgumentAmount) << ")";

		reportScriptError(x);
		return false;
	}

	return true;
}

int ScriptingObject::checkValidArguments(const var::NativeFunctionArgs &args)
{
	for (int i = 0; i < args.numArguments; i++)
	{
		if (args.arguments[i].isUndefined())
		{
			reportScriptError("Argument " + String(i) + " is undefined!");
			RETURN_IF_NO_THROW(i);
		}
	}

	return -1;
};



bool ScriptingObject::checkIfSynchronous(const Identifier &methodName) const
{
	const JavascriptMidiProcessor *sp = dynamic_cast<const JavascriptMidiProcessor*>(getScriptProcessor());

	if (sp == nullptr) return true; // HardcodedScriptProcessors are always synchronous

	if (sp->isDeferred())
	{
		reportScriptError("Illegal call of " + methodName.toString() + " (Can only be called in synchronous mode)");
	}

	return !sp->isDeferred();
}

void ScriptingObject::reportIllegalCall(const String &callName, const String &allowedCallback) const
{
	String x;
	x << "Call of " << callName << " outside of " << allowedCallback << " callback";

	reportScriptError(x);
};

String ValueTreeConverters::convertDynamicObjectToBase64(const var& object, const Identifier& id, bool compress)
{
	ValueTree v = convertDynamicObjectToValueTree(object, id);
	return convertValueTreeToBase64(v, compress);
}

ValueTree ValueTreeConverters::convertDynamicObjectToValueTree(const var& object, const Identifier& id)
{
	ValueTree v(id);

	if (object.isArray())
	{
		a2v_internal(v, id, *object.getArray());
		return v.getChild(0);
	}
	else
	{
		d2v_internal(v, "Data", object);
	}

	return v;
}

String ValueTreeConverters::convertValueTreeToBase64(const ValueTree& v, bool compress)
{
	MemoryOutputStream mos;

	if (compress)
	{
		GZIPCompressorOutputStream gzipper(&mos, 9);
		v.writeToStream(gzipper);
		gzipper.flush();
	}
	else
	{
		v.writeToStream(mos);
	}

	return mos.getMemoryBlock().toBase64Encoding();
}

var ValueTreeConverters::convertBase64ToDynamicObject(const String& base64String, bool isCompressed)
{
	auto v = convertBase64ToValueTree(base64String, isCompressed);
	return convertValueTreeToDynamicObject(v);
}

ValueTree ValueTreeConverters::convertBase64ToValueTree(const String& base64String, bool isCompressed)
{
	MemoryBlock mb;
	if (!mb.fromBase64Encoding(base64String))
	{
		jassertfalse;
		return ValueTree();
	}

	MemoryInputStream(mb, false);

	if (isCompressed)
	{
		auto v = ValueTree::readFromGZIPData(mb.getData(), mb.getSize());
		jassert(v.isValid());
		return v;
	}
	else
	{
		auto v = ValueTree::readFromData(mb.getData(), mb.getSize());
		jassert(v.isValid());
		return v;
	}
}

var ValueTreeConverters::convertValueTreeToDynamicObject(const ValueTree& v)
{
	jassert(v.isValid());

	var dData(new DynamicObject());

	v2d_internal(dData, v);
	return dData;
}

var ValueTreeConverters::convertFlatValueTreeToVarArray(const ValueTree& v)
{
	Array<var> ar;

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		auto child = v.getChild(i);

		auto obj = new DynamicObject();
		var d(obj);

		copyValueTreePropertiesToDynamicObject(v.getChild(i), d);

		ar.add(d);
	}

	return ar;
}

ValueTree ValueTreeConverters::convertVarArrayToFlatValueTree(const var& ar, const Identifier& rootId, const Identifier& childId)
{
	ValueTree v(rootId);

	if (auto a = ar.getArray())
	{
		for (auto value : *a)
		{
			ValueTree child(childId);
			copyDynamicObjectPropertiesToValueTree(child, value);
			v.addChild(child, -1, nullptr);
		}
	}

	return v;
}

void ValueTreeConverters::copyDynamicObjectPropertiesToValueTree(ValueTree& v, const var& obj, bool skipArray/*=false*/)
{
	if (auto obj_ = obj.getDynamicObject())
	{
		auto s = obj_->getProperties();

		for (int i = 0; i < s.size(); i++)
		{
			if (skipArray && s.getValueAt(i).isArray())
				continue;

			v.setProperty(s.getName(i), s.getValueAt(i), nullptr);
		}
	}
}

void ValueTreeConverters::copyValueTreePropertiesToDynamicObject(const ValueTree& v, var& obj)
{
	if (auto o = obj.getDynamicObject())
	{
		for (int i = 0; i < v.getNumProperties(); i++)
		{
			auto n = v.getPropertyName(i);
			o->setProperty(n, v.getProperty(n));
		}
	}
}

var ValueTreeConverters::convertContentPropertiesToDynamicObject(const ValueTree& v)
{
	static const Identifier ch("childComponents");

	auto vDyn = new DynamicObject();
	var vDynVar(vDyn);

	copyValueTreePropertiesToDynamicObject(v, vDynVar);

	auto list = Array<var>();

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		auto child = v.getChild(i);

		auto childVar = convertContentPropertiesToDynamicObject(child);

		list.add(childVar);
	}

	if (list.size() > 0)
	{
		vDyn->setProperty(ch, var(list));
	}

	return vDynVar;
}

ValueTree ValueTreeConverters::convertDynamicObjectToContentProperties(const var& d)
{
	static const Identifier ch("childComponents");

	ValueTree root;

	if (auto ar = d.getArray())
	{
		root = ValueTree("ContentProperties");

		for (auto child : *ar)
		{
			auto cTree = convertDynamicObjectToContentProperties(child);

			root.addChild(cTree, -1, nullptr);
		}

	}
	else if (d.getDynamicObject() != nullptr)
	{
		root = ValueTree("Component");

		copyDynamicObjectPropertiesToValueTree(root, d, true);

		auto childList = d.getProperty(ch, var());
		
		if (auto ar2 = childList.getArray())
		{
			for (auto child : *ar2)
			{
				auto cTree = convertDynamicObjectToContentProperties(child);

				root.addChild(cTree, -1, nullptr);
			}
		}
	}

	return root;
}

var ValueTreeConverters::convertScriptNodeToDynamicObject(ValueTree v)
{
	DynamicObject::Ptr root = new DynamicObject();

	Array<var> parameters;
	Array<var> nodes;

	// Convert Node Properties
	for (int i = 0; i < v.getNumProperties(); i++)
	{
		auto id = v.getPropertyName(i);
		root->setProperty(id, v[id]);
	}

	// Convert Node Parameters
	auto pTree = v.getChildWithName(scriptnode::PropertyIds::Parameters);

	for (auto p : pTree)
		parameters.add(convertValueTreeToDynamicObject(p));

	// Convert Child nodes
	auto nTree = v.getChildWithName(scriptnode::PropertyIds::Nodes);

	for (auto n : nTree)
	{
		nodes.add(convertScriptNodeToDynamicObject(n));
	}

	if (parameters.size() > 0)
		root->setProperty(scriptnode::PropertyIds::Parameters, parameters);
	if (nodes.size() > 0)
		root->setProperty(scriptnode::PropertyIds::Nodes, nodes);

	return var(root.get());
}

juce::ValueTree ValueTreeConverters::convertDynamicObjectToScriptNodeTree(var objVar)
{
	ValueTree t(scriptnode::PropertyIds::Node);

	if (auto obj = objVar.getDynamicObject())
	{
		// Convert Node Properties
		for (int i = 0; i < obj->getProperties().size(); i++)
		{
			auto id = obj->getProperties().getName(i);
			auto value = obj->getProperty(id);
			t.setProperty(id, value, nullptr);
		}

		ValueTree param(scriptnode::PropertyIds::Parameters);

		if (auto parameters = obj->getProperty(scriptnode::PropertyIds::Parameters).getDynamicObject())
		{
			for (int i = 0; i < parameters->getProperties().size(); i++)
			{
				auto id = parameters->getProperties().getName(i);
				auto value = parameters->getProperty(id);
				param.setProperty(id, value, nullptr);
			}
		}

		ValueTree nodes(scriptnode::PropertyIds::Nodes);

		if (auto nodeTree = obj->getProperty(scriptnode::PropertyIds::Nodes).getDynamicObject())
		{
			

			for (auto nt : nodeTree->getProperties())
			{
				auto childNode = convertDynamicObjectToScriptNodeTree(nt.value);
				nodes.addChild(childNode, -1, nullptr);
			}
		}

		t.addChild(nodes, -1, nullptr);
		t.addChild(param, -1, nullptr);
	}

	return t;
}

juce::var ValueTreeConverters::convertStringIfNumeric(const var& value)
{
	if (value.isString())
	{
		auto asString = value.toString();

		auto s = asString.begin();
		auto e = asString.end();

		int numNonNumeric = 0;
		int numDigits = 0;
		int numDots = 0;

		// Unfortunately, this will parse version numbers (1.0.0) as double,
		// so we need to do a better way of detecting double numbers...
		//if (asString.containsOnly("1234567890."))
		//	return var((double)value);

		// This beauty will be used in programming 101 books from now on...
		while (s != e)
		{
			auto isDigit = CharacterFunctions::isDigit(*s) || 
						   *s == '-'; // because of negative numbers, yo.

			numDigits += (int)isDigit;
			
			if (!isDigit)
			{
				numDots += (int)(*s == '.'); // because branching is for losers.
				numNonNumeric++;
			}
			
			// We've seen enough, thanks.
			if (numNonNumeric > 1)
				break;

			++s;
		}

		auto isAnIntForSure = numDigits != 0 && numNonNumeric == 0 && numDots == 0;

		auto isAnInt64ForSure = isAnIntForSure && (std::abs((int64)value) > (int64)(INT_MAX));

		auto isADoubleForSure = numDigits != 0 && numNonNumeric == 1 && numDots == 1;

		if (isAnInt64ForSure)
			return var((int64)value);

		if (isAnIntForSure)
			return var((int)value);

		if (isADoubleForSure)
			return var((double)value);
	}

	return value;
}

void ValueTreeConverters::v2d_internal(var& object, const ValueTree& v)
{
	if (isLikelyVarArray(v))
	{
		Array<var> childList;

		for (auto c : v)
		{
			if (c.getNumProperties() == 1 && c.hasProperty("value"))
				childList.add(convertStringIfNumeric(c["value"]));
			else
			{
				var childObj(new DynamicObject());
				v2d_internal(childObj, c);
				childList.add(childObj);
			}
		}

		object = var(childList);
	}
	else if (auto dyn = object.getDynamicObject())
	{
		auto& dynSet = dyn->getProperties();

		for (int i = 0; i < v.getNumProperties(); i++)
		{
			auto propId = v.getPropertyName(i);
			auto value = v.getProperty(propId);



			jassert(!value.isObject());
			dynSet.set(propId, convertStringIfNumeric(value));
		}

		for (int i = 0; i < v.getNumChildren(); i++)
		{
			auto child = new DynamicObject();
			var childVar(child);

			auto childTree = v.getChild(i);
			auto propId = childTree.getType();

			v2d_internal(childVar, childTree);

			dynSet.set(propId, childVar);
		}
	}
	else
	{
		jassertfalse;
	}
}

void ValueTreeConverters::d2v_internal(ValueTree& v, const Identifier& id, const var& object)
{
	if (auto dyn = object.getDynamicObject())
	{
		auto& dynSet = dyn->getProperties();

		for (int i = 0; i < dynSet.size(); i++)
		{
			auto v1 = dynSet.getValueAt(i);
			auto id1 = dynSet.getName(i);

			if (v1.isArray())
			{
				a2v_internal(v, id1, *v1.getArray());
			}
			else if (v1.isObject())
			{
				ValueTree child(dynSet.getName(i));

				d2v_internal(child, id1, v1);
				v.addChild(child, -1, nullptr);
			}
			else
			{
				v.setProperty(id1, v1, nullptr);
			}
		}
	}
	else
	{
		jassertfalse;
	}
}

void ValueTreeConverters::a2v_internal(ValueTree& v, const Identifier& id, const Array<var>& list)
{
	auto parentId = id;
	auto childId = parentId;

	ValueTree listParent(parentId);

	for (const auto& cv : list)
	{
		ValueTree child(childId);

		if (cv.isArray())
			a2v_internal(child, childId, *cv.getArray());
		else if (cv.isObject())
			d2v_internal(child, childId, cv);
		else
			child.setProperty("value", cv, nullptr);

		listParent.addChild(child, -1, nullptr);
	}

	v.addChild(listParent, -1, nullptr);
}

void ValueTreeConverters::v2a_internal(var& object, ValueTree& v, const Identifier& id)
{

}

bool ValueTreeConverters::isLikelyVarArray(const ValueTree& v)
{
	if (v.getNumChildren() == 0 || v.getNumProperties() != 0)
		return false;

	if (v.getNumChildren() == 1)
		return v.getType() == v.getChild(0).getType();

	auto firstId = v.getChild(0).getType();

	for (auto c : v)
	{
		if (c.getType() != firstId)
			return false;
	}

	return true;
}

WeakCallbackHolder::WeakCallbackHolder(ProcessorWithScriptingContent* p, ApiClass* parentObject, const var& callback, int numExpectedArgs_) :
	ScriptingObject(p),
	r(Result::ok()),
	numExpectedArgs(numExpectedArgs_)
{
	if (parentObject != nullptr)
		parentObject->addOptimizableFunction(callback);

	if (auto jp = dynamic_cast<JavascriptProcessor*>(p))
	{
		engineToUse = jp->getScriptEngine();
	}

	if (HiseJavascriptEngine::isJavascriptFunction(callback))
	{
		weakCallback = dynamic_cast<CallableObject*>(callback.getObject());
		
		weakCallback->storeCapturedLocals(capturedLocals, true);

		jassert(weakCallback != nullptr);

		// Store it ref-counted if the ref count is one to avoid deletion
		if (callback.getObject()->getReferenceCount() == 1)
			anonymousFunctionRef = callback;
	}
}



WeakCallbackHolder::WeakCallbackHolder(const WeakCallbackHolder& copy) :
	ScriptingObject(const_cast<ProcessorWithScriptingContent*>(copy.getScriptProcessor())),
	r(Result::ok()),
	weakCallback(copy.weakCallback),
	numExpectedArgs(copy.numExpectedArgs),
	highPriority(copy.highPriority),
	engineToUse(copy.engineToUse),
	anonymousFunctionRef(copy.anonymousFunctionRef),
	thisObject(copy.thisObject),
	refCountedThisObject(copy.refCountedThisObject),
	capturedLocals(copy.capturedLocals)
{
	args.addArray(copy.args);
}

WeakCallbackHolder::WeakCallbackHolder(WeakCallbackHolder&& other):
	ScriptingObject(other.getScriptProcessor()),
	r(other.r),
	weakCallback(other.weakCallback),
	numExpectedArgs(other.numExpectedArgs),
	highPriority(other.highPriority),
	anonymousFunctionRef(other.anonymousFunctionRef),
	engineToUse(other.engineToUse),
	refCountedThisObject(other.refCountedThisObject),
	thisObject(other.thisObject),
	capturedLocals(other.capturedLocals)
{
	args.swapWith(other.args);
}

WeakCallbackHolder::~WeakCallbackHolder()
{
	clear();
}

hise::WeakCallbackHolder& WeakCallbackHolder::operator=(WeakCallbackHolder&& other)
{
	r = other.r;
	weakCallback = other.weakCallback;
	numExpectedArgs = other.numExpectedArgs;
	highPriority = other.highPriority;
	anonymousFunctionRef = other.anonymousFunctionRef;
	engineToUse = other.engineToUse;
	refCountedThisObject = other.refCountedThisObject;
	thisObject = other.thisObject;
	capturedLocals = other.capturedLocals;
	args.swapWith(other.args);


	return *this;
}

hise::DebugInformationBase* WeakCallbackHolder::createDebugObject(const String& n) const
{
	if (weakCallback != nullptr)
	{
		return new ObjectDebugInformationWithCustomName(dynamic_cast<DebugableObjectBase*>(weakCallback.get()), (int)DebugInformation::Type::Callback, "%PARENT%." + n);
	}
	else
	{
		return new DebugInformation(DebugInformation::Type::Constant);
	}
}

void WeakCallbackHolder::addAsSource(DebugableObjectBase* sourceObject, const String& callbackId)
{
	if (weakCallback != nullptr)
	{
		auto id = sourceObject->getDebugName() + "." + callbackId;
		weakCallback->addAsSource(sourceObject, Identifier(id));
	}
}

void WeakCallbackHolder::clear()
{
	engineToUse = nullptr;
	weakCallback = nullptr;
	thisObject = nullptr;
	args.clear();

	decRefCount();
}

void WeakCallbackHolder::setThisObject(ReferenceCountedObject* thisObj)
{
	thisObject = dynamic_cast<DebugableObjectBase*>(thisObj);

	// Must call incRefCount before this method
	jassert(weakCallback == nullptr || (anonymousFunctionRef.isObject() || !weakCallback->allowRefCount()));
}

void WeakCallbackHolder::setThisObjectRefCounted(const var& t)
{
	refCountedThisObject = t;
}

bool WeakCallbackHolder::matches(const var& f) const
{
	return weakCallback == dynamic_cast<CallableObject*>(f.getObject());
}

void WeakCallbackHolder::reportError(const Result& r)
{
	if(!r.wasOk())
		debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), r.getErrorMessage());
}

juce::var WeakCallbackHolder::getThisObject()
{
	if (refCountedThisObject.isObject())
		return refCountedThisObject;

	if (auto d = dynamic_cast<ReferenceCountedObject*>(thisObject.get()))
	{
		return var(d);
	}

	return {};
}

void WeakCallbackHolder::call(var* arguments, int numArgs)
{
	call(var::NativeFunctionArgs(var(), arguments, numArgs));
}

void WeakCallbackHolder::call(const var::NativeFunctionArgs& args)
{
	try
	{
		if (weakCallback != nullptr && getScriptProcessor() != nullptr)
		{
			checkArguments("external call", args.numArguments, numExpectedArgs);
			auto copy = *this;
			copy.args.addArray(args.arguments, args.numArguments);

			{
				var::NativeFunctionArgs args_(var(), args.arguments, args.numArguments);
				checkValidArguments(args_);
			}
			
			auto t = highPriority ? JavascriptThreadPool::Task::HiPriorityCallbackExecution : JavascriptThreadPool::Task::LowPriorityCallbackExecution;
			getScriptProcessor()->getMainController_()->getJavascriptThreadPool().addJob(t, dynamic_cast<JavascriptProcessor*>(getScriptProcessor()), copy);
		}
		else
		{
			reportScriptError("function not found");
		}
	}
	catch (String& m)
	{
		debugError(dynamic_cast<Processor*>(getScriptProcessor()), m);
	}
}

Result WeakCallbackHolder::callSync(var* arguments, int numArgs, var* returnValue)
{
	auto a = var::NativeFunctionArgs(getThisObject(), arguments, numArgs);
	return callSync(a, returnValue);
}

Result WeakCallbackHolder::callSync(const var::NativeFunctionArgs& a, var* returnValue /*= nullptr*/)
{
	if (engineToUse.get() == nullptr || engineToUse->getRootObject() == nullptr)
	{
		clear();
        return Result::ok();
	}

	if (weakCallback.get() != nullptr)
	{
		if(!capturedLocals.isEmpty())
			weakCallback->storeCapturedLocals(capturedLocals, false);

		return weakCallback->call(engineToUse, a, returnValue);
	}
	else
		jassertfalse;

	return r;
}

juce::Result WeakCallbackHolder::operator()(JavascriptProcessor* p)
{
	jassert_locked_script_thread(getScriptProcessor()->getMainController_());

	var thisObj;

	if (auto d = dynamic_cast<ReferenceCountedObject*>(thisObject.get()))
		thisObj = var(d);

	auto r = callSync(args.getRawDataPointer(), args.size(), nullptr);

	if (!r.wasOk())
		debugError(dynamic_cast<Processor*>(p), r.getErrorMessage());

	return r;
}

var JSONConversionHelpers::convertBase64Data(const String& d, const ValueTree& cTree)
{
	if (d.isEmpty())
		return var();

	auto typeId = Identifier(cTree["type"].toString());

	if (typeId == ScriptingApi::Content::ScriptTable::getStaticObjectName())
		return Table::base64ToDataVar(d);
	if (typeId == ScriptingApi::Content::ScriptSliderPack::getStaticObjectName())
		return SliderPackData::base64ToDataVar(d);
	if (typeId == ScriptingApi::Content::ScriptAudioWaveform::getStaticObjectName())
		return var(d);

	return var();
}

String JSONConversionHelpers::convertDataToBase64(const var& d, const ValueTree& cTree)
{
	if (!d.isArray())
		return "";

	auto typeId = Identifier(cTree["type"].toString());

	if (typeId == ScriptingApi::Content::ScriptTable::getStaticObjectName())
		return Table::dataVarToBase64(d);
	if (typeId == ScriptingApi::Content::ScriptSliderPack::getStaticObjectName())
		return SliderPackData::dataVarToBase64(d);
	if (typeId == ScriptingApi::Content::ScriptAudioWaveform::getStaticObjectName())
		return d.toString();

	return "";
}


Result WeakCallbackHolder::CallableObject::call(HiseJavascriptEngine* engine, const var::NativeFunctionArgs& args, var* returnValue)
{
	if (thisAsRef == nullptr)
	{
		thisAsRef = dynamic_cast<ReferenceCountedObject*>(this);
		jassert(thisAsRef != nullptr);
	}

	auto rv = engine->callExternalFunction(var(thisAsRef), args, &lastResult, true);

	if (returnValue != nullptr)
		*returnValue = rv;

	return lastResult;
}

void ConstScriptingObject::gotoLocationWithDatabaseLookup()
{
	auto p = dynamic_cast<Processor*>(getScriptProcessor());
	
	if (auto loc = DebugableObject::Helpers::getLocationFromProvider(p, this))
	{
		DebugableObject::Helpers::gotoLocation(nullptr, dynamic_cast<JavascriptProcessor*>(p), loc);
	}
}

DynamicScriptingObject::DynamicScriptingObject(ProcessorWithScriptingContent* p):
	ScriptingObject(p)
{
	setMethod("exists", Wrappers::checkExists);
		
}

DynamicScriptingObject::~DynamicScriptingObject()
{}

String DynamicScriptingObject::getInstanceName() const
{ return name; }

bool DynamicScriptingObject::checkValidObject() const
{
	if(!objectExists())
	{
		reportScriptError(getObjectName().toString() + " " + getInstanceName() + " does not exist.");
		RETURN_IF_NO_THROW(false)
	}

	if(objectDeleted())
	{
		reportScriptError(getObjectName().toString() + " " + getInstanceName() + " was deleted");	
		RETURN_IF_NO_THROW(false)
	}

	return true;
}

void DynamicScriptingObject::setName(const String& name_) noexcept
{ name = name_; }

} // namespace hise

