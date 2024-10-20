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


namespace snex {
namespace jit {
using namespace juce;
USE_ASMJIT_NAMESPACE;

struct VectorMathFunction
{
	enum class ArgTypes
	{
		SingleBlock,
		Scalar,
		TwoBlocks,
		numArgTypes
	};

	static FunctionData* createSingleArgsFunction(void* ptr, const Identifier& id, ComplexType::Ptr blockType)
	{
		VectorMathFunction v(ptr, id);
		return v.create(blockType, ArgTypes::SingleBlock);
	}

	static FunctionData* createForScalar(void* ptr, const Identifier& id, ComplexType::Ptr blockType)
	{
		VectorMathFunction v(ptr, id);
		return v.create(blockType, ArgTypes::Scalar);
	}

	static FunctionData* createForTwoBlocks(void* ptr, const Identifier& id, ComplexType::Ptr blockType)
	{
		VectorMathFunction v(ptr, id);
		return v.create(blockType, ArgTypes::TwoBlocks);
	}

	VectorMathFunction(void* ptr, const Identifier& id_)
	{
		NamespacedIdentifier m("Math");
		id = m.getChildId(id_);
		f = ptr;
	}

	NamespacedIdentifier id;
	void* f;

	FunctionData* create(ComplexType::Ptr blockType, ArgTypes argTypes)
	{
		auto fData = new FunctionData();
		fData->id = id;
		fData->function = f;

		fData->returnType = TypeInfo(blockType, false, false);
		fData->args.add({ id.getChildId("b1"), TypeInfo(blockType, false, false) });

		if (argTypes == ArgTypes::Scalar)
			fData->args.add({ id.getChildId("s"), TypeInfo(Types::ID::Float, false, false) });
		else if(argTypes == ArgTypes::TwoBlocks)
			fData->args.add({ id.getChildId("b2"), TypeInfo(blockType, true, true) });

		return fData;
	}
};

#define FAST_DESCRIPTION(functionName, min, max) setDescription(juce::String("Calculates a fast approximation of the ") + String(functionName) + String(" function. The input range should not exceed [") + String(min) + " - " + String(max), { "input" }); 

#define DESCRIPTION(type, x) setDescription(juce::String("Calculates the ") + #type + " " + #x + " value", { "input" }); 
#define HNODE_JIT_VECTOR_FUNCTION_1(name) addFunction((void*)VectorMathFunction::createSingleArgsFunction(static_cast<block&(*)(block&)>(hmath::name), #name, blockType)); DESCRIPTION(name, block);
#define HNODE_JIT_VECTOR_FUNCTION(name) addFunction((void*)VectorMathFunction::createForTwoBlocks(static_cast<block&(*)(block&, const block&)>(hmath::name), #name, blockType)); DESCRIPTION(name, block);
#define HNODE_JIT_VECTOR_FUNCTION_S(name) addFunction((void*)VectorMathFunction::createForScalar(static_cast<block&(*)(block&, float)>(hmath::name), #name, blockType)); DESCRIPTION(name, float);

#define INT_TARGET INT_REG_W(d->target)
#define FP_TARGET FP_REG_W(d->target)
#define ARGS(i) d->args[i]
#define SETUP_MATH_INLINE(name) auto d = d_->toAsmInlineData(); \
								auto& cc = d->gen.cc; \
								auto type = d->target->getType(); \
								d->target->createRegister(cc); \
								cc.setInlineComment(name); \
								ignoreUnused(d, cc, type);


MathFunctions::MathFunctions(bool addInlinedFunctions, ComplexType::Ptr blockType) :
	FunctionClass(NamespacedIdentifier("Math"))
{
	jassert(blockType != nullptr);

	addFunctionConstant("PI", hmath::PI);
	addFunctionConstant("E", hmath::E);
	addFunctionConstant("SQRT2", hmath::SQRT2);
	addFunctionConstant("FORTYTWO", hmath::FORTYTWO);

#if 0
	HNODE_JIT_VECTOR_FUNCTION(min);
	HNODE_JIT_VECTOR_FUNCTION_S(min);
	HNODE_JIT_VECTOR_FUNCTION(max);
	HNODE_JIT_VECTOR_FUNCTION_S(max);
	HNODE_JIT_VECTOR_FUNCTION_1(abs);
#endif

	{
		auto p = new FunctionData();
		p->id = getClassName().getChildId("peak");
		p->returnType = Types::ID::Float;
		p->addArgs("b", TypeInfo(blockType, true, true));
		p->function = (void*)hmath::peakStatic;
		p->description = "Calculate the absolute peak value of the given block";
		p->object = nullptr;
		addFunction(p);
	}
    
	HNODE_JIT_ADD_C_FUNCTION_2(int, hmath::min, int, int, "min");		DESCRIPTION(int, smaller);
	HNODE_JIT_ADD_C_FUNCTION_2(int, hmath::max, int, int, "max");		DESCRIPTION(int, bigger);
	HNODE_JIT_ADD_C_FUNCTION_1(int, hmath::abs, int, "abs");			DESCRIPTION(int, positive);
	HNODE_JIT_ADD_C_FUNCTION_3(int, hmath::range, int, int, int, "range");		DESCRIPTION(int, clamped);
	HNODE_JIT_ADD_C_FUNCTION_2(int, hmath::fmod, int, int, "fmod");
	setDescription("Calculates the modulo (just a replacement for a % b", { "a", "b" });
	HNODE_JIT_ADD_C_FUNCTION_2(int, hmath::wrap, int, int, "wrap");
	setDescription("Wraps the first around the second limit (supports negative values)", { "a", "b" });

	HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::wrap, float, float, "wrap");
	setDescription("Wraps the first around the second limit (supports negative values)", { "a", "b" });

	HNODE_JIT_ADD_C_FUNCTION_2(double, hmath::wrap, double, double, "wrap");
	setDescription("Wraps the first around the second limit (supports negative values)", { "a", "b" });

	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sin, double, "sin");		DESCRIPTION(double, sin);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::asin, double, "asin");	DESCRIPTION(double, asin);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::cos, double, "cos");		DESCRIPTION(double, cos);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::acos, double, "acos");	DESCRIPTION(double, acos);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sinh, double, "sinh");	DESCRIPTION(double, sinh);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::cosh, double, "cosh");	DESCRIPTION(double, cosh);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::tan, double, "tan");		DESCRIPTION(double, tan);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::tanh, double, "tanh");	DESCRIPTION(double, tanh);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::atan, double, "atan");	DESCRIPTION(double, atan);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::atanh, double, "atanh");	DESCRIPTION(double, atanh);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::log, double, "log");		DESCRIPTION(double, log);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::log10, double, "log10");	DESCRIPTION(double, log10);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::exp, double, "exp");		DESCRIPTION(double, exp);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sqr, double, "sqr");		DESCRIPTION(double, sqr);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sqrt, double, "sqrt");	DESCRIPTION(double, sqrt);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::ceil, double, "ceil");	DESCRIPTION(double, ceil);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::floor, double, "floor");	DESCRIPTION(double, floor);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::round, double, "round");	DESCRIPTION(double, round);

	HNODE_JIT_ADD_C_FUNCTION_1(int, hmath::isinf, double, "isinf");
	setDescription("Returns true if the value is infinity", { "value" });
	HNODE_JIT_ADD_C_FUNCTION_1(int, hmath::isnan, double, "isnan");
	setDescription("Returns true if the value is NaN (invalid number)", { "value" });
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sanitize, double, "sanitize");
	setDescription("Sanitizes the number (inf & Nan => 0.0 / remove denormals", { "value" });

	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::fastsin, double, "fastsin");		FAST_DESCRIPTION("sin", -3.14, 3.14);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::fastsinh, double, "fastsinh");	FAST_DESCRIPTION("sinh", -5, 5.0);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::fastcos,  double, "fastcos");		FAST_DESCRIPTION("cos", -3.14, 3.14);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::fastcosh, double, "fastcosh");	FAST_DESCRIPTION("cosh", -5.0, 5.0);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::fasttanh, double, "fasttanh");	FAST_DESCRIPTION("tanh", -5.0, 5.0);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::fasttan,  double, "fasttan");		FAST_DESCRIPTION("tan", -3.14/2, 3.14/2);
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::fastexp,  double, "fastexp");		FAST_DESCRIPTION("exp", -6.0, 4.0);

	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::fastsin,  float, "fastsin");		FAST_DESCRIPTION("sin", -3.14, 3.14);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::fastsinh, float, "fastsinh");		FAST_DESCRIPTION("sinh", -5, 5.0);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::fastcos,  float, "fastcos");		FAST_DESCRIPTION("cos", -3.14, 3.14);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::fastcosh, float, "fastcosh");		FAST_DESCRIPTION("cosh", -5.0, 5.0);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::fasttanh, float, "fasttanh");		FAST_DESCRIPTION("tanh", -5.0, 5.0);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::fasttan,  float, "fasttan");		FAST_DESCRIPTION("tan", -3.14/2, 3.14/2);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::fastexp,  float, "fastexp");		FAST_DESCRIPTION("exp", -6.0, 4.0);
	
	HNODE_JIT_ADD_C_FUNCTION_2(double, hmath::pow, double, double, "pow");
	setDescription("Calculates the power", { "base", "exponent" });
	HNODE_JIT_ADD_C_FUNCTION_2(double, hmath::fmod, double, double, "fmod");
	setDescription("Wraps the input around the limit", { "input", "limit" });
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::db2gain, double, "db2gain");			
	setDescription("Converts a decibel value to a [0...1] gain value", { "decibelInput"});
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::gain2db, double, "gain2db");			
	setDescription("Converts a [0...1] gain value to a decibel value", { "gainInput" });
	HNODE_JIT_ADD_C_FUNCTION_3(double, hmath::map, double, double, double, "map");
	setDescription("Maps a [0...1] input value to the given range", { "input", "lowerLimit", "upperLimit" });
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sign, double, "sign");				
	setDescription("returns the sign (-1 or 1) of the input", { "input" });
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::abs, double, "abs");					
	setDescription("returns the absolute (positive) value of the input", { "input" });
	HNODE_JIT_ADD_C_FUNCTION_3(double, hmath::range, double, double, double, "range"); 
	setDescription("limits the input to the given range", { "input" , "lowerlimit", "upperlimit"});

	HNODE_JIT_ADD_C_FUNCTION_3(double, hmath::smoothstep, double, double, double, "smoothstep");
	setDescription("creates a smooth transition between upper and lower limit", { "input" , "lowerlimit", "upperlimit" });

	HNODE_JIT_ADD_C_FUNCTION_2(double, hmath::min, double, double, "min");			
	setDescription("returns the smaller value", { "firstValue", "secondValue" });
	HNODE_JIT_ADD_C_FUNCTION_2(double, hmath::max, double, double, "max");			
	setDescription("returns the bigger value", { "firstValue", "secondValue" });
	HNODE_JIT_ADD_C_FUNCTION_0(double, hmath::randomDouble, "randomDouble");		
	setDescription("returns a 64bit floating point random value", {});

	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sin, float, "sin");	DESCRIPTION(float, sin);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::asin, float, "asin");	DESCRIPTION(float, asin);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::cos, float, "cos");	DESCRIPTION(float, cos);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::acos, float, "acos");	DESCRIPTION(float, acos);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sinh, float, "sinh");	DESCRIPTION(float, sinh);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::cosh, float, "cosh");	DESCRIPTION(float, cosh);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::tan, float, "tan");	DESCRIPTION(float, tan);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::tanh, float, "tanh");	DESCRIPTION(float, tanh);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::atan, float, "atan");	DESCRIPTION(float, atan);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::atanh, float, "atanh");DESCRIPTION(float, atanh);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::log, float, "log");	DESCRIPTION(float, log);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::log10, float, "log10");DESCRIPTION(float, log10);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::exp, float, "exp");	DESCRIPTION(float, exp);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sqr, float, "sqr");	DESCRIPTION(float, sqr);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sqrt, float, "sqrt");	DESCRIPTION(float, sqrt);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::ceil, float, "ceil");	DESCRIPTION(float, ceil);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::floor, float, "floor");DESCRIPTION(float, floor);
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::round, float, "round");DESCRIPTION(float, round);

	HNODE_JIT_ADD_C_FUNCTION_1(int, hmath::isinf, float, "isinf");
	setDescription("Returns true if the value is infinity", { "value" });
	HNODE_JIT_ADD_C_FUNCTION_1(int, hmath::isnan, float, "isnan");
	setDescription("Returns true if the value is NaN (invalid number)", { "value" });
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sanitize, float, "sanitize");
	setDescription("Sanitizes the number (inf & Nan => 0.0 / remove denormals", { "value" });

	HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::pow, float, float, "pow");
	setDescription("Calculates the power", { "base", "exponent" });
	HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::fmod, float, float, "fmod");
	setDescription("Wraps the input around the limit", { "input", "limit" });
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::db2gain, float, "db2gain");
	setDescription("Converts a decibel value to a [0...1] gain value", { "decibelInput" });
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::gain2db, float, "gain2db");
	setDescription("Converts a [0...1] gain value to a decibel value", { "gainInput" });
	HNODE_JIT_ADD_C_FUNCTION_3(float, hmath::map, float, float, float, "map");
	setDescription("Maps a [0...1] input value to the given range", { "input", "lowerLimit", "upperLimit" });
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sign, float, "sign");
	setDescription("returns the sign (-1 or 1) of the input", { "input" });
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::abs, float, "abs");
	setDescription("returns the absolute (positive) value of the input", { "input" });
	HNODE_JIT_ADD_C_FUNCTION_3(float, hmath::range, float, float, float, "range");
	setDescription("limits the input to the given range", { "input" , "lowerlimit", "upperlimit" });

	HNODE_JIT_ADD_C_FUNCTION_3(float, hmath::smoothstep, float, float, float, "smoothstep");
	setDescription("creates a smooth transition between upper and lower limit", { "input" , "lowerlimit", "upperlimit" });

	HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::min, float, float, "min");
	setDescription("returns the smaller value", { "firstValue", "secondValue" });
	HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::max, float, float, "max");
	setDescription("returns the bigger value", { "firstValue", "secondValue" });
	HNODE_JIT_ADD_C_FUNCTION_0(float, hmath::random, "random");
	setDescription("returns a 32bit floating point random value", {});
	
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sig2mod, float, "sig2mod");
	setDescription("converts a -1...1 signal to a 0...1 mod signal", { "v" });

	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::mod2sig, float, "mod2sig");
	setDescription("converts a 0...1 mod signal to a -1..1 audio signal", { "v" });

	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sig2mod, double, "sig2mod");
	setDescription("converts a -1...1 signal to a 0...1 mod signal", { "v" });

	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::mod2sig, double, "mod2sig");
	setDescription("converts a 0...1 mod signal to a -1..1 audio signal", { "v" });

	HNODE_JIT_ADD_C_FUNCTION_3(float, hmath::norm, float, float, float, "norm");
	setDescription("normalises the input using the given range", { "input" , "lowerlimit", "upperlimit" });

	HNODE_JIT_ADD_C_FUNCTION_3(float, hmath::map, float, float, float, "map");
	setDescription("maps the normalised input to the output range", { "input" , "lowerlimit", "upperlimit" });

	HNODE_JIT_ADD_C_FUNCTION_3(double, hmath::norm, double, double, double, "norm");
	setDescription("normalises the input using the given range", { "input" , "lowerlimit", "upperlimit" });

	HNODE_JIT_ADD_C_FUNCTION_3(double, hmath::map, double, double, double, "map");
	setDescription("maps the normalised input to the output range", { "input" , "lowerlimit", "upperlimit" });

	using ScalarFunc = void*(*)(void*, float);
	using VectorFunc = void*(*)(void*, void*);

	HNODE_JIT_ADD_C_FUNCTION_2(void*, (ScalarFunc)hmath::vmuls, void*, float, "vmuls");
	HNODE_JIT_ADD_C_FUNCTION_2(void*, (ScalarFunc)hmath::vadds, void*, float, "vadds");
	HNODE_JIT_ADD_C_FUNCTION_2(void*, (ScalarFunc)hmath::vmovs, void*, float, "vmovs");

	HNODE_JIT_ADD_C_FUNCTION_2(void*, (VectorFunc)hmath::vmul, void*, void*, "vmul");
	HNODE_JIT_ADD_C_FUNCTION_2(void*, (VectorFunc)hmath::vadd, void*, void*, "vadd");
	HNODE_JIT_ADD_C_FUNCTION_2(void*, (VectorFunc)hmath::vsub, void*, void*, "vsub");
	HNODE_JIT_ADD_C_FUNCTION_2(void*, (VectorFunc)hmath::vmov, void*, void*, "vmov");

	for (auto f : functions)
		f->setConst(true);

	if (!addInlinedFunctions)
		return;

#if SNEX_ASMJIT_BACKEND
	addInliner("abs", Inliners::abs);
	addInliner("max", Inliners::max);
	addInliner("min", Inliners::min);
	addInliner("range", Inliners::range);
	addInliner("sign", Inliners::sign);
	addInliner("map", Inliners::map, Inliner::InlineType::HighLevel);
	addInliner("fmod", Inliners::fmod);
	addInliner("sin", Inliners::sin);
	addInliner("wrap", Inliners::wrap, Inliner::InlineType::HighLevel);
	addInliner("norm", Inliners::norm, Inliner::InlineType::HighLevel);
	addInliner("sig2mod", Inliners::sig2mod, Inliner::InlineType::HighLevel);
	addInliner("mod2sig", Inliners::mod2sig, Inliner::InlineType::HighLevel);
#endif

	for (auto& f : functions)
	{
		if (f->id.getIdentifier() == Identifier("fmod") && f->returnType == Types::ID::Integer)
		{
			f->inliner = Inliner::createHighLevelInliner(f->id, [](InlineData* b)
			{
				cppgen::Base c;
				c << "return a % b;";
				return SyntaxTreeInlineParser(b, { "a", "b" }, c).flush();
			});
		}

	}
}


void ConsoleFunctions::registerAllObjectFunctions(GlobalScope*)
{
	using namespace Types;


#if SNEX_MIR_BACKEND
	{
		auto f = createMemberFunction(Float, "print", { Float });
		f->setFunction(WrapperFloat::print);
		addFunction(f);
		setDescription("prints a float value to the console", { "value" });
	}

	{
		auto f = createMemberFunction(Double, "print", { Double });
		f->setFunction(WrapperDouble::print);
		setDescription("prints a double value to the console", { "value" });
		addFunction(f);
	}

	{
		auto f = createMemberFunction(Integer, "print", { Integer });
		f->setFunction(WrapperInt::print);
		setDescription("prints a integer value to the console", { "value" });
		addFunction(f);
	}
#endif


	{
		auto f = createMemberFunction(Types::ID::Void, "blink", {});

		
#if SNEX_ASMJIT_BACKEND
		f->inliner = Inliner::createAsmInliner(f->id, [this](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			int lineNumber = d->gen.location.getLine();

			FunctionData bf;
			bf.returnType = TypeInfo(Types::ID::Void);
			bf.addArgs("line", TypeInfo(Types::ID::Integer));
			bf.function = (void*)ConsoleFunctions::blink;
			bf.object = this;

			AsmCodeGenerator::TemporaryRegister tempReg(d->gen, d->target->getScope(), TypeInfo(Types::ID::Integer));

			tempReg.tempReg->createRegister(d->gen.cc);

			d->gen.cc.setInlineComment("blink at line");
			d->gen.cc.mov(INT_REG_W(tempReg.tempReg), lineNumber);

			d->args.add(tempReg.tempReg);
			return d->gen.emitFunctionCall(d->target, bf, d->object, d->args);
		});
#endif

		addFunction(f);
		setDescription("Sends a blink message to indicate that this was hit", {});
	}

	{
		auto f = createMemberFunction(Types::ID::Void, "stop", { Types::ID::Integer});
		
		f->setFunction(WrapperStop::stop);

#if SNEX_ASMJIT_BACKEND
		f->inliner = Inliner::createAsmInliner(f->id, [this](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			d->gen.location.calculatePosition(false);

			int lineNumber = d->gen.location.getLine();

			auto globalScope = d->args[0]->getScope()->getGlobalScope();
			auto& bpHandler = globalScope->getBreakpointHandler();

			auto& cc = d->gen.cc;

			auto data = reinterpret_cast<void*>(bpHandler.getLineNumber());

			auto lineReg = cc.newGpq();
			cc.mov(lineReg, (int64_t)data);
			auto target = x86::qword_ptr(lineReg);
			cc.setInlineComment("break at line");
			cc.mov(target, lineNumber);

			FunctionData rp;
			rp.function = (void*)WrapperStop::stop;
			rp.object = this;
			rp.returnType = TypeInfo(Types::ID::Void);
			rp.addArgs("condition", TypeInfo(Types::ID::Integer));

			return d->gen.emitFunctionCall(d->target, rp, d->object, d->args);
		});
#endif

		addFunction(f);
		setDescription("Breaks the execution if condition is true and dumps all variables", { "condition"});
	}

	{
		auto f = createMemberFunction(Types::ID::Void, "clear", { });
		f->setFunction(WrapperClear::clear);
		addFunction(f);
		setDescription("Dumps the current state of the class data", { });
	}

	{
		auto f = createMemberFunction(Types::ID::Void, "dump", { });
		f->setFunction(WrapperDump::dump);
		addFunction(f);
		setDescription("Dumps the current state of the class data", { });
	}

#if SNEX_ASMJIT_BACKEND
	{
		auto f = createMemberFunction(Types::ID::Void, "print", {});

		f->args.add(Symbol(f->id.getChildId("obj"), TypeInfo()));

		f->inliner = Inliner::createAsmInliner(f->id, [this](InlineData* b)
		{
			auto d = b->toAsmInlineData();

			auto typeToPrint = d->args[0]->getTypeInfo();

			if (typeToPrint.isComplexType())
			{
				auto ptr = typeToPrint.getComplexType();

				DumpItem* i = new DumpItem();

				i->type = ptr;
				i->id = d->args[0]->getVariableId().id;

				d->gen.location.calculatePosition(false, true);

				i->lineNumber = d->gen.location.getLine();

				dumpItems.add(i);

				FunctionData df;

				df.object = this;
				df.function = (void*)ConsoleFunctions::dumpObject;
				df.addArgs("ctype", TypeInfo(Types::ID::Integer, true));
				df.addArgs("dataPtr", TypeInfo(Types::ID::Pointer, true));
				df.returnType = TypeInfo(Types::ID::Void);

				AssemblyRegister::List args;

				{
					AsmCodeGenerator::TemporaryRegister a1(d->gen, d->args[0]->getScope(), TypeInfo(Types::ID::Integer));

					a1.tempReg->createRegister(d->gen.cc);
					d->gen.cc.mov(INT_REG_W(a1.tempReg), dumpItems.size() - 1);
					args.add(a1.tempReg);
					args.add(d->args[0]);
				}

				return d->gen.emitFunctionCall(d->target, df, d->object, args);
			}
			else
			{
				auto nativeType = typeToPrint.getType();

				if (typeToPrint.isRef())
					d->args[0]->loadMemoryIntoRegister(d->gen.cc);

				d->gen.location.calculatePosition(false, true);

				auto lineNumber = d->gen.location.getLine();

				AsmCodeGenerator::TemporaryRegister tmp(d->gen, d->target->getScope(), Types::ID::Integer);

				tmp.tempReg->setImmediateValue(lineNumber);

				d->args.add(tmp.tempReg);

				FunctionData fToCall;
				fToCall.returnType = TypeInfo(Types::ID::Void);
				fToCall.addArgs("args", nativeType);
				fToCall.addArgs("lineNumber", Types::ID::Integer);
				fToCall.object = this;

				switch (nativeType)
				{
				case Types::ID::Double: fToCall.function = (void*)WrapperDouble::print; break;
				case Types::ID::Float: fToCall.function = (void*)WrapperFloat::print; break;
				case Types::ID::Integer: fToCall.function = (void*)WrapperInt::print; break;
				}

				return d->gen.emitFunctionCall(d->target, fToCall, d->object, d->args);
			}
		});

		addFunction(f);
		setDescription("Dumps the given object / expression", { "object" });
	}
#endif
}


struct Funky
{
	Funky(InlineData* b) : b_(b), args({}) { resolveType(); };
	Funky(InlineData* b, const StringArray& arguments) : b_(b), args(arguments) { resolveType(); }
	Funky(InlineData* b, const String& a1) : b_(b), args(StringArray::fromTokens(a1, ",", "")) { args.trim(); resolveType(); };

	Funky& operator<<(double v)
	{
		currentLine << getLiteral(v);
		return *this;
	}

	Funky& operator<<(const String& s)
	{
		currentLine << s;

		if (s.contains(";"))
			flushLine(false);

		return *this;
	}

	void flushLine(bool addMissingSemicolon = true)
	{
		if (addMissingSemicolon && currentLine.isNotEmpty() && !currentLine.endsWith(";"))
			currentLine << ";";

		c << currentLine;
		currentLine = {};
	}

	Result flush()
	{
		if (currentLine.isNotEmpty())
			flushLine();

		return SyntaxTreeInlineParser(b_, args, c).flush();
	}

	operator Result()
	{
		return flush();
	}

private:

	void resolveType()
	{
		auto st = b_->toSyntaxTreeData();

		if (st->target != nullptr)
			type = st->target->getType();

		if (type == Types::ID::Dynamic || type == Types::ID::Void)
		{
			for (auto a : b_->toSyntaxTreeData()->args)
			{
				type = a->getType();

				if (type != Types::ID::Dynamic)
					break;
			}
		}
	}

	String getLiteral(double value)
	{
		VariableStorage v(type, var(value));
		return Types::Helpers::getCppValueString(value);
	}

	Types::ID type = Types::ID::Dynamic;
	String currentLine;
	InlineData* b_;
	cppgen::Base c;
	StringArray args;
};


#if SNEX_ASMJIT_BACKEND
juce::Result MathFunctions::Inliners::abs(InlineData* d_)
{
	SETUP_MATH_INLINE("inline abs");

	IF_(int)
	{
		cc.mov(INT_TARGET, INT_REG_R(ARGS(0)));
		cc.neg(INT_TARGET);
		cc.cmovl(INT_TARGET, INT_REG_R(ARGS(0)));
	}

	IF_(float)
	{
        auto cv = Data128::fromU32(0x7fffffff);
		auto c = cc.newConst(asmjit::ConstPoolScope::kGlobal, cv.getData(), cv.size());

		FP_OP(cc.movss, d->target, ARGS(0));

		
		cc.andps(FP_TARGET, c);
	}
	IF_(double)
	{
        auto cv = Data128::fromU64(0x7fffffffffffffff);
		auto c = cc.newConst(asmjit::ConstPoolScope::kGlobal, cv.getData(), cv.size());
		cc.movsd(FP_TARGET, FP_REG_R(ARGS(0)));
		cc.andps(FP_TARGET, c);
	}

	return Result::ok();
}

juce::Result MathFunctions::Inliners::max(InlineData* d_)
{
	SETUP_MATH_INLINE("inline max");

	IF_(float)
	{
		FP_OP(cc.movss, d->target, ARGS(0));
		FP_OP(cc.maxss, d->target, ARGS(1));
	}
	IF_(double)
	{
		FP_OP(cc.movsd, d->target, ARGS(0));
		FP_OP(cc.maxsd, d->target, ARGS(1));
	}
	IF_(int)
	{
		ARGS(0)->loadMemoryIntoRegister(cc);

		INT_OP(cc.mov, d->target, ARGS(0));
		INT_OP(cc.cmp, ARGS(0),		ARGS(1));

		if (ARGS(1)->isImmediate())
			ARGS(1)->loadMemoryIntoRegister(cc);

		INT_OP_WITH_MEM(cc.cmovl, d->target, ARGS(1));
	}

	return Result::ok();
}

juce::Result MathFunctions::Inliners::min(InlineData* d_)
{
	SETUP_MATH_INLINE("inline min");

	IF_(float)
	{
		FP_OP(cc.movss, d->target, ARGS(0));
		FP_OP(cc.minss, d->target, ARGS(1));
	}
	IF_(double)
	{
		FP_OP(cc.movsd, d->target, ARGS(0));
		FP_OP(cc.minsd, d->target, ARGS(1));
	}
	IF_(int)
	{
		ARGS(0)->loadMemoryIntoRegister(cc);

		INT_OP(cc.mov, d->target, ARGS(0));
		INT_OP(cc.cmp, ARGS(0), ARGS(1));
		INT_OP_WITH_MEM(cc.cmovg, d->target, ARGS(1));
	}

	return Result::ok();
}

juce::Result MathFunctions::Inliners::range(InlineData* d_)
{
	SETUP_MATH_INLINE("inline range");

	IF_(float)
	{
		FP_OP(cc.movss, d->target, ARGS(0));
		FP_OP(cc.maxss, d->target, ARGS(1));
		FP_OP(cc.minss, d->target, ARGS(2));

	}
	IF_(double)
	{
		FP_OP(cc.movsd, d->target, ARGS(0));
		FP_OP(cc.maxsd, d->target, ARGS(1));
		FP_OP(cc.minsd, d->target, ARGS(2));
	}
	IF_(int)
	{
		auto v = ARGS(0);
		auto l = ARGS(1);
		auto u = ARGS(2);
		auto rv = d->target;
		rv->createRegister(cc);

		v->loadMemoryIntoRegister(cc);
		l->loadMemoryIntoRegister(cc);
		u->loadMemoryIntoRegister(cc);

		Intrinsics::range(cc, INT_REG_W(rv), INT_REG_R(v), INT_REG_R(l), INT_REG_R(u));
	}

	return Result::ok();
}

juce::Result MathFunctions::Inliners::sign(InlineData* d_)
{
	SETUP_MATH_INLINE("inline sign");

	ARGS(0)->loadMemoryIntoRegister(cc);

	auto pb = cc.newLabel();
	auto nb = cc.newLabel();

	IF_(float)
	{
		auto input = FP_REG_R(ARGS(0));
		auto t = FP_REG_W(d->target);
        
        auto cv = Data128::fromF32(-1.0f, 1.0f);
        
        auto mem = cc.newConst(ConstPoolScope::kGlobal, cv.getData(), cv.size());
		auto i = cc.newGpq();
		auto r2 = cc.newGpq();
		auto zero = cc.newXmmSs();

		cc.xorps(zero, zero);
		cc.xor_(i, i);
		cc.ucomiss(input, zero);
		cc.seta(i.r8());
		cc.lea(r2, mem);
		cc.movss(t, x86::ptr(r2, i, 2, 0, 4));
	}
	IF_(double)
	{
		auto input = FP_REG_R(ARGS(0));
		auto t = FP_REG_W(d->target);
        
        auto cv = Data128::fromF64(-1.0, 1.0);
        
        jassert(cv.size() == 16);
        
		auto mem = cc.newConst(ConstPoolScope::kGlobal, cv.getData(), cv.size());
		auto i = cc.newGpq();
		auto r2 = cc.newGpq();
		auto zero = cc.newXmmSd();

		cc.xorps(zero, zero);
		cc.xor_(i, i);
		cc.ucomisd(input, zero);
		cc.seta(i.r8());
		cc.lea(r2, mem);
		cc.movsd(t, x86::ptr(r2, i, 3, 0, 8));
	}

	return Result::ok();
}


juce::Result MathFunctions::Inliners::map(InlineData* b)
{
	Funky c(b, "v, minValue, maxValue");
	c << "return v * (maxValue - minValue) + minValue";
	return c;
}

juce::Result MathFunctions::Inliners::norm(InlineData* b)
{
	Funky c(b, "v, minValue, maxValue");
	{
		c << "return (v - minValue) / (maxValue - minValue)";
	}
	
	return c;
}

juce::Result MathFunctions::Inliners::sig2mod(InlineData* b)
{
	Funky c(b, "v");
	{
		c << "return v * " << 0.5 << " + " << 0.5;
	}

	return c;
}

juce::Result MathFunctions::Inliners::mod2sig(InlineData* b)
{
	Funky c(b, "v");
	{
		c << "return v * " << 2.0 << "-" << 1.0;
	}
	
	return c;
}

juce::Result MathFunctions::Inliners::wrap(InlineData* b)
{
	Funky c(b, "value, limit");
	{
		c << "auto w = Math.fmod(value, limit);";
		c << "return value >= " << 0.0 << " ? w : Math.fmod(limit + w, limit)";
	}
	
	return c;
}

juce::Result MathFunctions::Inliners::fmod(InlineData* d_)
{
	SETUP_MATH_INLINE("inline fmod");

	X86Mem x = d->gen.createFpuMem(ARGS(0));
	X86Mem y = d->gen.createFpuMem(ARGS(1));
	auto u = d->gen.createStack(d->target->getType());

	cc.fld(y);
	cc.fld(x);
	cc.fprem();
	cc.fstp(x);
	cc.fstp(u);

	d->gen.writeMemToReg(d->target, x);

	return Result::ok();
}

static constexpr int TableSize = 1024;
static float sinTable[TableSize];
static double sinTableDouble[TableSize];

juce::Result MathFunctions::Inliners::sin(InlineData* d_)
{
	SETUP_MATH_INLINE("inline sin");

	for (int i = 0; i < TableSize; i++)
	{
		double v = ((double)i / (double)TableSize) * double_Pi * 2.0;

		v = std::sin(v);

		sinTableDouble[i] = v;
		sinTable[i] = (float)v;
	}

	auto i1 = cc.newGpq();
	auto i2 = cc.newGpq();
	

	d->target->loadMemoryIntoRegister(cc);
	
	x86::Xmm tmpFloat;
	x86::Xmm idx;
	x86::Mem tableSize;

	bool isDouble = d->args[0]->getType() == Types::ID::Double;

	if (isDouble)
	{
		idx = cc.newXmmSd();
		tmpFloat = cc.newXmmSd();
		tableSize = cc.newDoubleConst(ConstPoolScope::kGlobal, (double)TableSize / (2.0 * double_Pi));

		if (d->args[0]->isMemoryLocation())
			cc.movsd(idx, d->args[0]->getAsMemoryLocation());
		else
			cc.movsd(idx, FP_REG_R(d->args[0]));

		cc.mulsd(idx, tableSize);
		cc.cvttsd2si(i1, idx);
		cc.cvtsi2sd(tmpFloat, i1);
		cc.subsd(idx, tmpFloat);
	}
	else
	{
		idx = cc.newXmmSs();
		tmpFloat = cc.newXmmSs();
		tableSize = cc.newFloatConst(ConstPoolScope::kGlobal, (float)TableSize / (2.0f * float_Pi));

		if (d->args[0]->isMemoryLocation())
			cc.movss(idx, d->args[0]->getAsMemoryLocation());
		else
			cc.movss(idx, FP_REG_R(d->args[0]));

		cc.mulss(idx, tableSize);
		cc.cvttss2si(i1, idx);
		cc.cvtsi2ss(tmpFloat, i1);
		cc.subss(idx, tmpFloat);
	}

	cc.lea(i2, x86::ptr(i1).cloneAdjusted(1));
	cc.and_(i1, TableSize - 1);
	cc.and_(i2, TableSize - 1);

	auto adr = isDouble ? (uint64_t)sinTableDouble : (uint64_t)sinTable;

	auto dataReg = cc.newGpq();
	cc.mov(dataReg, adr);
	auto p1 = x86::ptr(dataReg, i1, isDouble ? 3 : 2).cloneResized(isDouble ? 8 : 4);
	auto p2 = x86::ptr(dataReg, i2, isDouble ? 3 : 2).cloneResized(isDouble ? 8 : 4);

	auto v1 = FP_REG_W(d->target);

	X86Xmm d1;

	if (isDouble)
	{
		d1 = cc.newXmmSd(); cc.movsd(d1, p1);
		cc.movsd(v1, p2);
		cc.subsd(v1, d1);
		cc.mulsd(v1, idx);
		cc.addsd(v1, d1);
	}
	else
	{
		d1 = cc.newXmmSs(); cc.movss(d1, p1);
		cc.movss(v1, p2);
		cc.subss(v1, d1);
		cc.mulss(v1, idx);
		cc.addss(v1, d1);
	}

	return Result::ok();
}

#endif

}
}
