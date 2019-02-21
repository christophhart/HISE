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

#ifndef MODULEBROWSER_H_INCLUDED
#define MODULEBROWSER_H_INCLUDED

namespace hise { using namespace juce;

class ModuleBrowser : public SearchableListComponent
{
public:

	ModuleBrowser(BackendRootWindow* rootWindow);

	SET_GENERIC_PANEL_ID("ModuleBrowser");

	class ModuleItem : public SearchableListComponent::Item
	{
	public:

		enum DragState
		{
			Inactive = 0,
			Illegal,
			Legal,
			numDragStates
		};

		ModuleItem(String name_, Identifier id_);;

		void paint(Graphics& g) override;

		void mouseEnter(const MouseEvent&) override;
		void mouseUp(const MouseEvent&) override;
		void mouseExit(const MouseEvent&) override { repaint(); }

		int getPopupHeight() const override { return parameters.size() * 20 + 30; }

		void paintPopupBox(Graphics &g) const;

		void setDragState(DragState newState) { state = newState; repaint(); }

		DragState getDragState() const { return state; }

		void setParameters(const ValueTree &v);

	private:

		friend class ParameterListComponent;

		Identifier id;
		String name;
		DragState state;

		StringArray parameters;
	};

	class ModuleCollection : public SearchableListComponent::Collection
	{
	public:

		enum Types
		{
			ModulatorSynths,
			MidiProcessors,
			VoiceStartModulators,
			TimeVariantModulators,
			EnvelopeModulators,
			Effects,
			numTypes
		};

		ModuleCollection(Types t);

		void paint(Graphics &g) override;

	private:

		Path p;

		String typeName;

		ScopedPointer<FactoryType> factoryType;

		Colour c;
		Colour c2;

		ValueTree vt;

	};

	int getNumCollectionsToCreate() const override
	{
		return ModuleCollection::numTypes;
	}

	Collection *createCollection(int index) override
	{
		return new ModuleCollection((ModuleCollection::Types)index);
	}
};

} // namespace hise

#endif  // MODULEBROWSER_H_INCLUDED
