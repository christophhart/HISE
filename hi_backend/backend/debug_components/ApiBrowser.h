/*
  ==============================================================================

    ApiBrowser.h
    Created: 24 Feb 2016 3:43:51pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef APIBROWSER_H_INCLUDED
#define APIBROWSER_H_INCLUDED


class ApiCollection : public SearchableListComponent
{
public:

	ApiCollection(BaseDebugArea *area);

	class MethodItem : public SearchableListComponent::Item
	{
	public:

		MethodItem(const ValueTree &methodTree_, const String &className_);

		int getPopupHeight() const override	{ return 150; }

		void paintPopupBox(Graphics &g) const
		{
			help.draw(g, Rectangle<float>(10.0f, 10.0f, 280.0f, (float)getPopupHeight() - 20));
		}

		void mouseEnter(const MouseEvent&) override { repaint(); }
		void mouseExit(const MouseEvent&) override { repaint(); }
		void mouseDoubleClick(const MouseEvent&) override;

		bool keyPressed(const KeyPress& key) override;

		void paint(Graphics& g) override;

	private:

		void insertIntoCodeEditor();

		AttributedString help;

		String name;
		String description;
		String className;
		String arguments;

		const ValueTree methodTree;
	};

	class ClassCollection : public SearchableListComponent::Collection
	{
	public:
		ClassCollection(const ValueTree &api);;

		void paint(Graphics &g) override;
	private:

		String name;

		const ValueTree classApi;
	};

	int getNumCollectionsToCreate() const override { return apiTree.getNumChildren(); }

	Collection *createCollection(int index) override
	{
		return new ClassCollection(apiTree.getChild(index));
	}

private:

	ValueTree apiTree;

	BaseDebugArea *parentArea;

};

#endif  // APIBROWSER_H_INCLUDED
