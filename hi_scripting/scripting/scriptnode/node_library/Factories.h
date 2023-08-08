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

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace dll
{

/** This factory will create the project nodes using the dll. */
struct BackendHostFactory : public scriptnode::NodeFactory
{
	BackendHostFactory(DspNetwork* n, dll::ProjectDll::Ptr dll);

	Identifier getId() const override
	{
		RETURN_STATIC_IDENTIFIER("project");
	}

	DynamicLibraryHostFactory dllFactory;
};

}

namespace jdsp
{
class Factory : public NodeFactory
{
public:
	Factory(DspNetwork* network);
	Identifier getId() const override { return "jdsp"; };
};
}

namespace analyse
{

class Factory : public NodeFactory
{
public:

	Factory(DspNetwork* network);;
	Identifier getId() const override { return "analyse"; }
};
}

namespace examples
{

class Factory : public NodeFactory
{
public:

	Factory(DspNetwork* network);;
	Identifier getId() const override { return "examples"; }
};

}

namespace control
{
	class Factory : public NodeFactory
	{
	public:
		Factory(DspNetwork* network);
		Identifier getId() const override { return "control"; };
	};
}


namespace envelope
{
class Factory : public NodeFactory
{
public:

	Factory(DspNetwork* network);;
	Identifier getId() const override { return "envelope"; }
};
}

namespace generator
{
	class Factory : public NodeFactory
	{
	public:

		Factory(DspNetwork* network);;
		Identifier getId() const override { return "generator"; }
	};
}


namespace core
{

class Factory : public NodeFactory
{
public:

	Factory(DspNetwork* network);;
	Identifier getId() const override { return "core"; }
};

}

namespace fx
{

class Factory : public NodeFactory
{
public:

	Factory(DspNetwork* network);;
	Identifier getId() const override { return "fx"; }
};

}

namespace dynamics
{

struct Factory : public NodeFactory
{
	Factory(DspNetwork* network);;
	Identifier getId() const override { return "dynamics"; }
};
}



namespace filters
{

struct Factory : public NodeFactory
{
	Factory(DspNetwork* n);;
	Identifier getId() const override { return "filters"; }
};

}

namespace math
{

class Factory : public NodeFactory
{
public:

	Factory(DspNetwork* n);

	Identifier getId() const override { return "math"; }
};
}

namespace routing
{
class Factory : public NodeFactory
{
public:
	Factory(DspNetwork* n);

	Identifier getId() const override { return "routing"; }
};
}

}
