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
	FunctionClass({ {"Math"}, Types::ID::Void })
{
	addFunctionConstant("PI", hmath::PI);
	addFunctionConstant("E", hmath::E);
	addFunctionConstant("SQRT2", hmath::SQRT2);
	addFunctionConstant("FORTYTWO", hmath::FORTYTWO);

#define DESCRIPTION(type, x) setDescription(juce::String("Calculates the ") + #type + " " + #x + " value", { "input" }); 

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
	HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::min, float, float, "min");
	setDescription("returns the smaller value", { "firstValue", "secondValue" });
	HNODE_JIT_ADD_C_FUNCTION_2(float, hmath::max, float, float, "max");
	setDescription("returns the bigger value", { "firstValue", "secondValue" });
	HNODE_JIT_ADD_C_FUNCTION_0(float, hmath::random, "random");
	setDescription("returns a 32bit floating point random value", {});
}

MessageFunctions::MessageFunctions() :
	FunctionClass({ {"Message"}, Types::ID::Void })
{
	HNODE_JIT_ADD_C_FUNCTION_1(int, Wrapper::getNoteNumber, HiseEvent, "getNoteNumber");
	HNODE_JIT_ADD_C_FUNCTION_1(int, Wrapper::getVelocity, HiseEvent, "getVelocity");
	HNODE_JIT_ADD_C_FUNCTION_1(int, Wrapper::getChannel, HiseEvent, "getChannel");
	HNODE_JIT_ADD_C_FUNCTION_1(double, Wrapper::getFrequency, HiseEvent, "getFrequency");
}

BlockFunctions::BlockFunctions() :
	FunctionClass({ {"Block"}, Types::ID::Void })
{
	HNODE_JIT_ADD_C_FUNCTION_2(float, Wrapper::getSample, block, int, "getSample");
	setDescription("Returns the sample at the given index", { "internal", "sampleIndex" });
	HNODE_JIT_ADD_C_FUNCTION_3(void, Wrapper::setSample, block, int, float, "setSample");
	setDescription("Sets the sample at the given index", { "internal", "sampleIndex", "value" });

	//HNODE_JIT_ADD_C_FUNCTION_1(AddressType, Wrapper::getWritePointer, block, "getWritePointer");
	HNODE_JIT_ADD_C_FUNCTION_1(int, Wrapper::size, block, "size");
	setDescription("returns the size of the buffer", { "internal" });
}


void ConsoleFunctions::registerAllObjectFunctions(GlobalScope*)
{
	using namespace Types;

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

	{
		auto f = createMemberFunction(Event, "print", { Event });
		f->setFunction(WrapperEvent::print);
		setDescription("prints an event text representation to the console", { "value" });
		addFunction(f);
	}

	{
		auto f = createMemberFunction(Types::ID::Void, "stop", { Types::ID::Integer});
		f->setFunction(WrapperStop::stop);
		addFunction(f);
		setDescription("Breaks the execution if condition is true and dumps all variables", { "condition"});
	}
}

void WrappedBufferBase::rightClickCallback(const MouseEvent& e, Component* c)
{
	auto* g = new Graph(true);

	AudioSampleBuffer bf;
	auto f = b.getData();

	bf.setDataToReferTo(&f, 1, b.size());

	g->setSize(300, 300);
	g->setBuffer(bf);
	g->setCurrentPosition(getCurrentPosition());


	auto top = c->getTopLevelComponent();

	CallOutBox::launchAsynchronously(g, {12, 12, 50, 50}, top);

}

}
}
