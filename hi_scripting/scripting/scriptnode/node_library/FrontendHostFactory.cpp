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

FrontendHostFactory::FrontendHostFactory(DspNetwork* n) :
	NodeFactory(n)
{
	if ((staticFactory = dynamic_cast<scriptnode::dll::StaticLibraryHostFactory*>(scriptnode::DspNetwork::createStaticFactory())))
	{
		int numNodes = staticFactory->getNumNodes();

		for (int i = 0; i < numNodes; i++)
		{
			NodeFactory::Item item;

			item.id = Identifier(staticFactory->getId(i));
			item.cb = [this, i](DspNetwork* p, ValueTree v)
			{
				if (staticFactory->isInterpretedNetwork(i))
				{
					zstd::ZDefaultCompressor comp;
					ValueTree cv;
					MemoryBlock mb;
					mb.fromBase64Encoding(staticFactory->items[i].networkData);
					comp.expand(mb, cv);

					jassert(cv.isValid());

					auto useMod = staticFactory->items[i].isModNode;

					if (useMod)
						return scriptnode::HostHelpers::initNodeWithNetwork<InterpretedModNode>(p, v, cv, useMod);
					else
						return scriptnode::HostHelpers::initNodeWithNetwork<InterpretedNode>(p, v, cv, useMod);
				}
				else
				{
					auto nodeId = staticFactory->getId(i);

					if (staticFactory->getWrapperType(i) == 1)
					{
						auto t = new scriptnode::InterpretedModNode(p, v);
						t->initFromDll(staticFactory, i, true);
						return dynamic_cast<NodeBase*>(t);
					}

					else
					{
						auto t = new scriptnode::InterpretedNode(p, v);
						t->initFromDll(staticFactory, i, false);
						return dynamic_cast<NodeBase*>(t);
					}
				}
			};

			monoNodes.add(item);
		}
	}
}



} // namespace hise

