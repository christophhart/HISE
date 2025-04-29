#pragma once

namespace hise
{
using namespace juce;


class RectangleDynamicObject: public DynamicObject,
						      public DebugableObjectBase
{
	struct FunctionMap
	{
		using Args = var::NativeFunctionArgs;

		FunctionMap();

		bool contains(const Identifier& id) const { return functions.find(id) != functions.end(); }

		var invoke(const Identifier& id, const Args& a);

	private:

		static var create(const Args& a, Rectangle<double> s);

		static bool getRectangleArgs(const Args& a, Rectangle<double> & r);

		static bool getPointArgs(const Args& a, Point<double> & p);

		static double getDoubleArgs(const var::NativeFunctionArgs& a, int index, double defaultValue=0.0);
		static RectangleDynamicObject* getObject(const var::NativeFunctionArgs& a);
		static Rectangle<double>& getRectangle(const var::NativeFunctionArgs& a);

		std::map<Identifier, var::NativeFunction> functions;
	};

protected:

	RectangleDynamicObject(Rectangle<double> d={}):
	  rectangle(d)
	{}

public:

	virtual ~RectangleDynamicObject() {};

	bool hasProperty (const Identifier& propertyName) const override;
	const var& getProperty (const Identifier& propertyName) const override;
	void setProperty (const Identifier& propertyName, const var& newValue) override;
	void removeProperty (const Identifier& propertyName) override {}
    bool hasMethod (const Identifier& methodName) const override;
	var invokeMethod (Identifier methodName, const var::NativeFunctionArgs& args) override;
	Ptr clone() override;

	

	String toString() const { return rectangle.toString(); }
	void writeAsJSON (OutputStream& mos, int indentLevel, bool, int) override;

	Rectangle<double> getRectangle() const { return rectangle; }

	

protected:

	Rectangle<double> rectangle;
	SharedResourcePointer<FunctionMap> functionMap;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RectangleDynamicObject);
};


}