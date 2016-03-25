/*
  ==============================================================================

    ModuleBrowser.h
    Created: 24 Feb 2016 3:43:59pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef MODULEBROWSER_H_INCLUDED
#define MODULEBROWSER_H_INCLUDED


class ModuleBrowser : public SearchableListComponent
{
public:

	ModuleBrowser(BaseDebugArea *area);

	class ModuleItem : public SearchableListComponent::Item,
					   public DragAndDropContainer
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

		void mouseDrag(const MouseEvent&) override;
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



#endif  // MODULEBROWSER_H_INCLUDED
