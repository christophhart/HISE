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
namespace jit {
using namespace juce;

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
	}
};

static void mirError(MIR_error_type_t error_type, const char* format, ...)
{
	DBG(format);
	throw String(format);
}


MirObject::MirObject() :
	r(Result::fail("nothing compiled"))
{
	ctx = MIR_init();
#if JUCE_WINDOWS // don't want to bother...
	MIR_set_error_func(ctx, mirError);
#endif
}



juce::Result MirObject::compileMirCode(const ValueTree& ast)
{
	MirBuilder b(ctx, ast);

	r = b.parse();

	if (r.wasOk())
	{
#if CREATE_MIR_TEXT
		return compileMirCode(b.getMirText());
#else
		modules.add(b.getModule());

		MIR_load_module(ctx, b.getModule());

		MIR_gen_init(ctx, 1);
		MIR_gen_set_optimize_level(ctx, 1, 3);
		MIR_link(ctx, MIR_set_gen_interface, NULL);

		{
			auto f = fopen("D:\\test.txt", "w");

			MIR_output(ctx, f);

			fclose(f);
		}
		
		auto mf = File("D:\\test.txt");

		auto mirCode = mf.loadFileAsString();
		DBG(mirCode);

		mf.deleteFile();

#endif
	}

	return r;
}

juce::Result MirObject::compileMirCode(const String& code)
{
	if (SyntaxTreeExtractor::isBase64Tree(code))
	{
		auto v = SyntaxTreeExtractor::getSyntaxTree(code);
		return compileMirCode(v);
	}

	r = Result::ok();

	try
	{
		MIR_scan_string(ctx, code.getCharPointer().getAddress());

		if (auto m = DLIST_TAIL(MIR_module_t, *MIR_get_module_list(ctx)))
		{
			modules.add(m);
			MIR_load_module(ctx, m);
			MIR_gen_init(ctx, 1);
			MIR_gen_set_optimize_level(ctx, 1, 3);
			MIR_link(ctx, MIR_set_gen_interface, NULL);
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

	return r;
}



FunctionData MirObject::operator[](const String& functionName)
{
	MIR_item_t f, main_func;

	main_func = nullptr;

	for (auto& m : modules)
	{
		for (auto f = DLIST_HEAD(MIR_item_t, m->items); f != NULL; f = DLIST_NEXT(MIR_item_t, f))
		{
			if (f->item_type == MIR_func_item && strcmp(f->u.func->name, functionName.getCharPointer().getAddress()) == 0)
			{
				


				main_func = f;
				break;
			}
		}
	}

	if (main_func)
	{
		FunctionData f;
		f.id = NamespacedIdentifier(functionName);

		auto x = main_func->u.func;

		if(x->nres != 0)
			f.returnType = TypeInfo(MirHelpers::getTypeFromMirEnum(x->res_types[0]), false, false);

		for (int i = 0; i < x->nargs; i++)
		{
			auto v = x->vars->varr[i];
			f.addArgs(v.name, TypeInfo(MirHelpers::getTypeFromMirEnum(v.type)));
		}

		
		
		

		f.function = MIR_gen(ctx, 0, main_func);
		
		return f;
	}
	else
	{
		r = Result::fail("Can't find function with id " + functionName);
		return {};
	}
}

juce::Result MirObject::getLastError() const
{
	return r;
}

MirObject::~MirObject()
{
	MIR_finish(ctx);
}

void MirObject::example()
{
	String code = "\
	m_loop: module\n\
	loop:   func i64, i64:limit # a comment\n\
	\n\
	        local i64:count\n\
	        mov count, 0\n\
	        bge L1, count, limit\n\
	L2:     # a separate label\n\
	        add count, count, 1; blt L2, count, limit\n\
	L1:     mul count, count, 24\n\
	        ret count  # label with insn\n\
	        endfunc\n\
	        endmodule\n";

	using FuncType = int64_t(*)(int64_t);

	MirObject obj;
	auto ok = obj.compileMirCode(code);

	if (ok.wasOk())
	{

		auto f = obj["loop"];

		auto res = f.call<int>(12);

		
		int x = 5;
	}
}




}
}
