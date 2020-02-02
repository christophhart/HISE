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
using namespace asmjit;

MathFunctions::MathFunctions() :
	FunctionClass("Math")
{
	addFunctionConstant("PI", hmath::PI);
	addFunctionConstant("E", hmath::E);
	addFunctionConstant("SQRT2", hmath::SQRT2);
	addFunctionConstant("FORTYTWO", hmath::FORTYTWO);

	HNODE_JIT_ADD_C_FUNCTION_1(double, std_::sin, double, "sin");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::asin, double, "asin");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::cos, double, "cos");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::acos, double, "acos");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sinh, double, "sinh");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::cosh, double, "cosh");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::tan, double, "tan");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::tanh, double, "tanh");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::atan, double, "atan");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::atanh, double, "atanh");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::log, double, "log");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::log10, double, "log10");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::exp, double, "exp");
	HNODE_JIT_ADD_C_FUNCTION_2(double, hmath::pow, double, double, "pow");
	HNODE_JIT_ADD_C_FUNCTION_2(double, hmath::fmod, double, double, "fmod");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sqr, double, "sqr");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sqrt, double, "sqrt");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::ceil, double, "ceil");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::floor, double, "floor");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::db2gain, double, "db2gain");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::gain2db, double, "gain2db");
	HNODE_JIT_ADD_C_FUNCTION_3(double, hmath::map, double, double, double, "map");

	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::sign, double, "sign");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::abs, double, "abs");
	HNODE_JIT_ADD_C_FUNCTION_1(double, hmath::round, double, "round");
	HNODE_JIT_ADD_C_FUNCTION_3(double, hmath::range, double, double, double, "range");
	HNODE_JIT_ADD_C_FUNCTION_2(double, hmath::min, double, double, "min");
	HNODE_JIT_ADD_C_FUNCTION_2(double, hmath::max, double, double, "max");
	HNODE_JIT_ADD_C_FUNCTION_0(double, hmath::randomDouble, "randomDouble");


	HNODE_JIT_ADD_C_FUNCTION_1(float, std_::sinf, float, "sin");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::asin, float, "asin");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::cos, float, "cos");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::acos, float, "acos");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sinh, float, "sinh");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::cosh, float, "cosh");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::tan, float, "tan");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::tanh, float, "tanh");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::atan, float, "atan");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::atanh, float, "atanh");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::log, float, "log");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::log10, float, "log10");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::exp, float, "exp");
	HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::pow, float, float, "pow");
	HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::fmod, float, float, "fmod");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sqr, float, "sqr");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sqrt, float, "sqrt");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::ceil, float, "ceil");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::db2gain, float, "db2gain");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::gain2db, float, "gain2db");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::floor, float, "floor");
	HNODE_JIT_ADD_C_FUNCTION_3(float, hmath::map, float, float, float, "map");

	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::sign, float, "sign");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::abs, float, "abs");
	HNODE_JIT_ADD_C_FUNCTION_1(float, hmath::round, float, "round");
	HNODE_JIT_ADD_C_FUNCTION_3(float, hmath::range, float, float, float, "range");
	HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::min, float, float, "min");
	HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::max, float, float, "max");
	HNODE_JIT_ADD_C_FUNCTION_0(float, hmath::random, "random");
}

MessageFunctions::MessageFunctions() :
	FunctionClass("Message")
{
	HNODE_JIT_ADD_C_FUNCTION_1(int, Wrapper::getNoteNumber, HiseEvent, "getNoteNumber");
	HNODE_JIT_ADD_C_FUNCTION_1(int, Wrapper::getVelocity, HiseEvent, "getVelocity");
	HNODE_JIT_ADD_C_FUNCTION_1(int, Wrapper::getChannel, HiseEvent, "getChannel");
	HNODE_JIT_ADD_C_FUNCTION_1(double, Wrapper::getFrequency, HiseEvent, "getFrequency");
}

BlockFunctions::BlockFunctions() :
	FunctionClass("Block")
{
	HNODE_JIT_ADD_C_FUNCTION_2(float, Wrapper::getSample, block, int, "getSample");
	HNODE_JIT_ADD_C_FUNCTION_3(void, Wrapper::setSample, block, int, float, "setSample");
	HNODE_JIT_ADD_C_FUNCTION_1(AddressType, Wrapper::getWritePointer, block, "getWritePointer");
	HNODE_JIT_ADD_C_FUNCTION_1(int, Wrapper::size, block, "size");
}


void ConsoleFunctions::registerAllObjectFunctions(GlobalScope*)
{
	using namespace Types;

	{
		auto f = createMemberFunction(Float, "print", { Float });
		f->setFunction(WrapperFloat::print);
		addFunction(f);
	}

	{
		auto f = createMemberFunction(Double, "print", { Double });
		f->setFunction(WrapperDouble::print);
		addFunction(f);
	}

	{
		auto f = createMemberFunction(Integer, "print", { Integer });
		f->setFunction(WrapperInt::print);
		addFunction(f);
	}

	{
		auto f = createMemberFunction(Event, "print", { Event });
		f->setFunction(WrapperEvent::print);
		addFunction(f);
	}
}

}
}
