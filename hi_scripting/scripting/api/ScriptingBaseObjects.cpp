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
	return processor;
};

const ProcessorWithScriptingContent *ScriptingObject::getScriptProcessor() const
{
	return processor;
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

	
	d2v_internal(v, "Data", object);

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

	DynamicObject::Ptr d = new DynamicObject();
	var dData = var(d);

	v2d_internal(dData, v);
	return dData;
}

var ValueTreeConverters::convertFlatValueTreeToVarArray(const ValueTree& v)
{
	Array<var> ar;

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		auto child = v.getChild(i);

		DynamicObject::Ptr obj = new DynamicObject();
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

	DynamicObject::Ptr vDyn = new DynamicObject();
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

	return var(root);
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

void ValueTreeConverters::v2d_internal(var& object, const ValueTree& v)
{
	if (auto dyn = object.getDynamicObject())
	{
		auto& dynSet = dyn->getProperties();

		for (int i = 0; i < v.getNumProperties(); i++)
		{
			auto propId = v.getPropertyName(i);
			auto value = v.getProperty(propId);

			jassert(!value.isObject());

			dynSet.set(propId, value);
		}

		for (int i = 0; i < v.getNumChildren(); i++)
		{
			DynamicObject::Ptr child = new DynamicObject();
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

void ValueTreeConverters::d2v_internal(ValueTree& v, const Identifier& /*id*/, const var& object)
{
	if (auto dyn = object.getDynamicObject())
	{
		auto& dynSet = dyn->getProperties();

		for (int i = 0; i < dynSet.size(); i++)
		{
			auto v1 = dynSet.getValueAt(i);
			auto id1 = dynSet.getName(i);

			if (v1.isObject())
			{
				ValueTree child(dynSet.getName(i));

				d2v_internal(child, id1, v1);
				v.addChild(child, -1, nullptr);
			}
			else
			{
				jassert(!v1.isArray());
				v.setProperty(id1, v1, nullptr);
			}
		}
	}
	else
	{
		jassertfalse;
	}
}

WeakCallbackHolder::WeakCallbackHolder(ProcessorWithScriptingContent* p, const var& callback, int numExpectedArgs_) :
	ScriptingObject(p),
	r(Result::ok()),
	numExpectedArgs(numExpectedArgs_)
{
	if (auto jp = dynamic_cast<JavascriptProcessor*>(p))
	{
		engineToUse = jp->getScriptEngine();
	}

	if (HiseJavascriptEngine::isJavascriptFunction(callback))
	{
		weakCallback = dynamic_cast<DebugableObjectBase*>(callback.getObject());

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
	thisObject(copy.thisObject)
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
	thisObject(other.thisObject)
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
	thisObject = other.thisObject;
	args.swapWith(other.args);

	return *this;
}

void WeakCallbackHolder::clear()
{
	engineToUse = nullptr;
	weakCallback = nullptr;
	thisObject = nullptr;
	args.clear();

	decRefCount();
}

void WeakCallbackHolder::call(var* arguments, int numArgs)
{
	try
	{
		if (weakCallback != nullptr)
		{
			checkArguments("external call", numArgs, numExpectedArgs);
			auto copy = *this;
			copy.args.addArray(arguments, numArgs);
			var::NativeFunctionArgs args_(var(), arguments, numArgs);
			checkValidArguments(args_);
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

juce::Result WeakCallbackHolder::operator()(JavascriptProcessor* p)
{
	jassert_locked_script_thread(getScriptProcessor()->getMainController_());

	if (engineToUse.get() == nullptr)
	{
		clear();
		return Result::fail("Engine is dangling");
	}

	if (weakCallback.get() != nullptr)
	{
		var thisObj;

		if (auto d = dynamic_cast<ReferenceCountedObject*>(thisObject.get()))
			thisObj = var(d);

		var::NativeFunctionArgs a(thisObj, args.getRawDataPointer(), args.size());
		engineToUse->callExternalFunction(var(dynamic_cast<ReferenceCountedObject*>(weakCallback.get())), a, &r);

		if (!r.wasOk())
			debugError(dynamic_cast<Processor*>(p), r.getErrorMessage());
	}
	else
		jassertfalse;

	return r;
}

} // namespace hise