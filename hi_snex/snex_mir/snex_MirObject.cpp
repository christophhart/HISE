/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/



#include "src/mir.h"
#include "src/mir-gen.h"


namespace snex {
namespace mir {
using namespace juce;

[[maybe_unused]] static void mirError(MIR_error_type_t error_type, const char* format, ...)
{
	DBG(format);
	jassertfalse;
	throw String(format);
}

struct MirHelpers
{
	static Types::ID getTypeFromMirEnum(int t)
	{
		if (t == MIR_T_I64 || t == MIR_T_U64)
		{
			return Types::ID::Integer;
		}
		if (t == MIR_T_F)
		{
			return Types::ID::Float;
		}
		if (t == MIR_T_D)
		{
			return Types::ID::Double;
		}
		if (t == MIR_T_P)
		{
			return Types::ID::Pointer;
		}
        
        return Types::ID::Void;
	}
};

struct MirFunctionCollection : public jit::FunctionCollectionBase
{
	MirFunctionCollection();

	~MirFunctionCollection();

	FunctionData getFunction(const NamespacedIdentifier& functionId) override;

	virtual VariableStorage getVariable(const Identifier& id) const override
	{
		for (auto& a : dataIds)
		{
			if (a.id.getIdentifier() == id)
			{
				switch (a.typeInfo.getType())
				{
				case Types::ID::Integer:	return VariableStorage(getData<int>(id.toString()));
				case Types::ID::Float:		return VariableStorage(getData<float>(id.toString()));
				case Types::ID::Double:		return VariableStorage(getData<double>(id.toString()));
				default: jassertfalse;		return VariableStorage();
				}
			}
		}

		return VariableStorage();
	}

	void* getVariablePtr(const Identifier& id) const override
	{
		return dataItems[id.toString()];
	}

	size_t getMainObjectSize() const override
	{
		jassertfalse;
		return 0;
	}

	juce::String dumpTable() override
	{
		return {};
	}

	Array<NamespacedIdentifier> getFunctionIds() const override
	{
		return allFunctions;
	}

	virtual Array<jit::Symbol> getAllVariables() const
	{
		return dataIds;
	}

	template <typename T> T getData(const String& dataId, size_t byteOffset = 0) const
	{
		if (dataItems.contains(dataId))
		{
			auto bytePtr = reinterpret_cast<uint8*>(dataItems[dataId]);
			bytePtr += byteOffset;

			return *reinterpret_cast<T*>(bytePtr);
		}

		jassertfalse;
		return T();
	}

	ValueTree getDataLayout(int dataIndex) const override
	{
		auto c = globalData.getChild(dataIndex);

		if (c.isValid())
		{
			return fillLayoutWithData(c["ObjectId"].toString(), c);
		}

		return {};
	}

	ValueTree fillLayoutWithData(const String& dataId, const ValueTree& valueData) const;

	void fillRecursive(ValueTree& copy, const String& dataId, size_t offset) const;

	Array<NamespacedIdentifier> allFunctions;

	Array<Symbol> dataIds;

	MIR_context* ctx;
	Array<MIR_module*> modules;

	HashMap<String, void*> dataItems;

	std::map<String, FunctionData> functionMap;

	ValueTree globalData;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MirFunctionCollection);
};

MirFunctionCollection::MirFunctionCollection()
{
	ctx = MIR_init();
#if JUCE_WINDOWS // don't want to bother...
	MIR_set_error_func(ctx, mirError);
#endif
}

MirFunctionCollection::~MirFunctionCollection()
{
	

	MIR_finish(ctx);
}

snex::jit::FunctionData MirFunctionCollection::getFunction(const NamespacedIdentifier& functionId)
{
	auto functionName = TypeConverters::NamespacedIdentifier2MangledMirVar(functionId);

	if (!allFunctions.contains(NamespacedIdentifier(functionName)))
	{
		return {};
	}

	return functionMap[functionName];
}

void MirFunctionCollection::fillRecursive(ValueTree& d, const String& dataId, size_t offset) const
{
	if (d.getType() == Identifier("NativeType"))
	{
		auto type = Types::Helpers::getTypeFromTypeName(d["type"].toString());

		if (type == Types::ID::Integer)
			d.setProperty("Value", getData<int>(dataId, offset), nullptr);
		if (type == Types::ID::Float)
			d.setProperty("Value", getData<float>(dataId, offset), nullptr);
		if (type == Types::ID::Double)
			d.setProperty("Value", getData<double>(dataId, offset), nullptr);

		return;
	}

	for (int i = 0; i < d.getNumChildren(); i++)
	{
		auto m = d.getChild(i);

		if (m.getType() == Identifier("TemplateParameter") ||
			m.getType() == Identifier("Method"))
		{
			d.removeChild(i--, nullptr);
		}
	}

	auto bytePtr = reinterpret_cast<uint8*>(dataItems[dataId]);
	bytePtr += offset;

	for (auto m : d)
	{
		auto o = (size_t)(int)m["offset"];

		auto type = SimpleTypeParser(m["type"].toString()).getTypeInfo().getType();





		if (type == Types::ID::Integer)
			m.setProperty("Value", getData<int>(dataId, offset + o), nullptr);
		if (type == Types::ID::Float)
			m.setProperty("Value", getData<float>(dataId, offset + o), nullptr);
		if (type == Types::ID::Double)
			m.setProperty("Value", getData<double>(dataId, offset + o), nullptr);
		if (type == Types::ID::Pointer)
		{
			auto dl = m.getChildWithName("DataLayout");

			if (dl.isValid())
			{
				fillRecursive(dl, dataId, offset + o);
			}
			else
			{
				auto p = reinterpret_cast<uint64>(getData<void*>(dataId, offset + o));

				auto hexPtr = "0x" + String::toHexString(p);

				m.setProperty("Value", hexPtr, nullptr);
			}
		}

		m.setProperty("address", "0x" + String::toHexString(reinterpret_cast<uint64>(bytePtr) + o), nullptr);
	}
}

ValueTree MirFunctionCollection::fillLayoutWithData(const String& dataId, const ValueTree& valueData) const
{
	auto copy = valueData.createCopy();

	fillRecursive(copy, dataId, 0);

	return copy;
}



void* MirCompiler::currentConsole = nullptr;
Array<StaticFunctionPointer> MirCompiler::currentFunctions;

MirCompiler::MirCompiler(jit::GlobalScope& m):
	r(Result::fail("nothing compiled")),
	memory(m)
{
	currentConsole = m.getGlobalFunctionClass(NamespacedIdentifier("Console"));
	
}

snex::jit::FunctionCollectionBase* MirCompiler::compileMirCode(const ValueTree& ast)
{
	if (currentFunctionClass == nullptr)
		currentFunctionClass = new MirFunctionCollection();

	MirBuilder b(getFunctionClass()->ctx, ast);

    b.setDataLayout(dataLayout);
    
	r = b.parse();

	if (r.wasOk())
	{
        auto code = b.getMirText();
		auto ok = compileMirCode(code);
        
        getFunctionClass()->globalData = b.getGlobalData();
        
        return ok;
	}

	return nullptr;
}

snex::mir::MirFunctionCollection* MirCompiler::getFunctionClass()
{
	return dynamic_cast<MirFunctionCollection*>(currentFunctionClass.get());
}



void MirCompiler::setLibraryFunctions(const Array<StaticFunctionPointer>& functionMap)
{
    currentFunctions.clearQuick();
    
	currentFunctions.addArray(functionMap);
}

void* MirCompiler::resolve(const char* name)
{
	

	String label(name);

	if (label == "Console")
		return (void*)MirCompiler::currentConsole;

	if (label == "PolyHandler_getVoiceIndexStatic_ip")
		return (void*)PolyHandler::getVoiceIndexStatic;
	if(label == "PolyHandler_getSizeStatic_ip")
		return (void*)PolyHandler::getSizeStatic;

	for (const auto& f : currentFunctions)
	{
		if (f.label == label)
			return f.function;
	}

    for(const auto& f: currentFunctions)
    {
        DBG(f.label);
        ignoreUnused(f);
    }
    
	

	jassertfalse;
	return nullptr;
}



bool MirCompiler::isExternalFunction(const String& sig)
{
	for (const auto& f : currentFunctions)
	{
		if (f.signature == sig)
			return true;
	}

	return false;
}

snex::jit::FunctionCollectionBase* MirCompiler::compileMirCode(const String& code)
{
    if (SyntaxTreeExtractor::isBase64Tree(code))
	{
		auto v = SyntaxTreeExtractor::getSyntaxTree(code);
		return compileMirCode(v);
	}

	if (currentFunctionClass == nullptr)
		currentFunctionClass = new MirFunctionCollection();

	assembly = code;

	r = Result::ok();

	try
	{
        File fs;
        
		auto ctx = getFunctionClass()->ctx;

		MIR_scan_string(ctx, code.getCharPointer().getAddress());

		if (auto m = DLIST_TAIL(MIR_module_t, *MIR_get_module_list(ctx)))
		{
			getFunctionClass()->modules.add(m);
			MIR_load_module(ctx, m);
			MIR_gen_init(ctx, 1);
			MIR_gen_set_optimize_level(ctx, 1, 3);
            //MIR_gen_set_debug_file(ctx, 1, dbgfile);
			MIR_link(ctx, MIR_set_gen_interface, &MirCompiler::resolve);
            
            for(auto& m: getFunctionClass()->modules)
            {
                for (auto f = DLIST_HEAD(MIR_item_t, m->items); f != NULL; f = DLIST_NEXT(MIR_item_t, f))
                {
                    if(f->item_type == MIR_data_item)
                    {
                        String s(f->u.data->name);
                        
                        if(s.isNotEmpty() && !s.startsWithChar('.'))
                        {
                            getFunctionClass()->dataItems.set(s, f->addr);

							NamespacedIdentifier id(s);
							TypeInfo t(Types::ID::Integer, false, false);

                            getFunctionClass()->dataIds.add(jit::Symbol(id, t));
                        }
                    }
					else if (f->item_type == MIR_func_item)
					{
						String s(f->u.data->name);

						s = s.upToLastOccurrenceOf("_", false, false);

						if (s.isNotEmpty())
						{
							NamespacedIdentifier id(s);
							getFunctionClass()->allFunctions.add(id);
						}

						auto main_func = f;
						
						jit::FunctionData fd;
						fd.id = NamespacedIdentifier::fromString(s);

						auto x = main_func->u.func;

						if (x->nres != 0)
							fd.returnType = TypeInfo(MirHelpers::getTypeFromMirEnum(x->res_types[0]), false, false);
						else
							fd.returnType = Types::ID::Void;

						for (uint32 i = 0; i < x->nargs; i++)
						{
							auto v = x->vars->varr[i];
							fd.addArgs(v.name, TypeInfo(MirHelpers::getTypeFromMirEnum(v.type)));
						}

						fd.function = MIR_gen(ctx, 0, main_func);

						if (fd.function != nullptr)
							fd.function = main_func->u.func->machine_code;

						fd.numBytes = main_func->u.func->num_bytes;

						getFunctionClass()->functionMap.emplace(s, fd);
					}
                }
            }

			MIR_gen_finish(ctx);

			return getFunctionClass();
		}
		else
		{
			r = Result::fail("Can't find module");
		}
	}
	catch (String& error)
	{
		r = Result::fail(error);
	}

	return nullptr;

}



juce::Result MirCompiler::getLastError() const
{
	return r;
}

void MirCompiler::setDataLayout(const Array<ValueTree>& data)
{
    dataLayout = data;
}

MirCompiler::~MirCompiler()
{
	
}







}
}
