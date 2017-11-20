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

void multiply(float* dst, const float* src, int numValues)
{

	FloatVectorOperations::multiply(dst, src, numValues);
}

void multiplyScalar(float* dst, double scalar, int numValues)
{
	FloatVectorOperations::multiply(dst, (float)scalar, numValues);
}

TccDspObject::TccDspObject(const File &f_) :
compiledOk(false),
pb(nullptr),
pp(nullptr),
sp(nullptr),
gp(nullptr),
gnp(nullptr),
it(nullptr),
rl(nullptr),
f(f_)
{
	context = new TccContext(f);
	context->openContext();

	String code = f.loadFileAsString();

	if (code.isNotEmpty() && context->compile(code.getCharPointer()) == 0)
	{
		pb = (Signatures::processBlock)context->getFunction("processBlock");
		pp = (Signatures::prepareToPlay)context->getFunction("prepareToPlay");
		sp = (Signatures::setParameter)context->getFunction("setParameter");
		gp = (Signatures::getParameter)context->getFunction("getParameter");
		gnp = (Signatures::getNumParameters)context->getFunction("getNumParameters");
		it = (Signatures::initialise)context->getFunction("initialise");
		rl = (Signatures::release)context->getFunction("release");

		compiledOk = true;
	}

	context->closeContext();

	if(it != nullptr) it();
}


TccDspObject::~TccDspObject()
{
	if (rl != nullptr) rl();
}

var TccDspFactory::createModule(const String &module) const
{
	DspInstance* p = new DspInstance(this, module);

	try
	{
		p->initialise();
	}
	catch (String errorMessage)
	{
		DBG(errorMessage);
		return var::undefined();
	}

	return var(p);
}

DspBaseObject * TccDspFactory::createDspBaseObject(const String &module) const
{
	File directory = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Scripts);

	File f = directory.getChildFile(module + ".c");

	if (f.existsAsFile())
	{
		const String code = f.loadFileAsString();

		return new TccDspObject(f);
	}
	else
	{
		throw String(module + ".c was not found");
		return nullptr;
	}
}

void TccDspFactory::destroyDspBaseObject(DspBaseObject *object) const
{
	if (object != nullptr) delete object;
}


} // namespace hise