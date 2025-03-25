namespace hise { using namespace juce;

//#define PRINTX(x, argString) DBG("/** Add description for " + String(#x) + "*/"); DBG("void " + String(#x) + argString + " { jassertfalse; }");
#define PRINTX(x, argsString)

#define ADD_FUNCTION(x, argString, body) PRINTX(x, argString); functions[Identifier(#x)] = body;
#define ADD_FUNCTION1(func, argString) ADD_FUNCTION(func, argString, [](const Args& a) { return create(a, getRectangle(a).func(getDoubleArgs(a, 0))); });
#define ADD_FUNCTION2(func, argString) ADD_FUNCTION(func, argString, [](const Args& a) { return create(a, getRectangle(a).func(getDoubleArgs(a, 0), getDoubleArgs(a, 1))); });

#define ADD_VOID_FUNCTION2(func, argString) ADD_FUNCTION(func, argString, [](const Args& a) { getRectangle(a).func(getDoubleArgs(a, 0), getDoubleArgs(a, 1)); return var(); });

RectangleDynamicObject::FunctionMap::FunctionMap()
{
	ADD_FUNCTION1(removeFromTop, "(double numToRemove)");
	ADD_FUNCTION1(removeFromLeft, "(double numToRemove)");
	ADD_FUNCTION1(removeFromRight, "(double numToRemove)");
	ADD_FUNCTION1(removeFromBottom, "(double numToRemove)");

	ADD_FUNCTION1(withX, "(double newX)");
	ADD_FUNCTION1(withY, "(double newY)");
	ADD_FUNCTION1(withLeft, "(double newLeft)");
	ADD_FUNCTION1(withRight, "(double newRight)");
	ADD_FUNCTION1(withBottom, "(double newBottom)");
	ADD_FUNCTION1(withBottomY, "(double newBottomY)");
	ADD_FUNCTION1(withWidth, "(double newWidth)");
	ADD_FUNCTION1(withHeight, "(double newHeight)");

	ADD_FUNCTION(withCentre, "(double newCentreX, double newCentreY)", [](const Args& a) { return create(a, getRectangle(a).withCentre({getDoubleArgs(a, 0), getDoubleArgs(a, 1)})); });;
	ADD_FUNCTION2(withSize, "(double newWidth, double newHeight)");
	ADD_FUNCTION2(withSizeKeepingCentre, "(double newWidth, double newHeight)");
	ADD_FUNCTION2(translated, "(double deltaX, double deltaY)");
	
	ADD_FUNCTION1(withTrimmedBottom, "(double amountToRemove)");
	ADD_FUNCTION1(withTrimmedLeft, "(double amountToRemove)");
	ADD_FUNCTION1(withTrimmedRight, "(double amountToRemove)");
	ADD_FUNCTION1(withTrimmedTop, "(double amountToRemove)");

	ADD_FUNCTION(scaled, "(double factorX, double optionalFactorY", [](const Args& a)
	{
		if(a.numArguments == 2)
			return create(a, getRectangle(a).transformedBy(AffineTransform::scale(getDoubleArgs(a, 0), getDoubleArgs(a, 1))));
		if(a.numArguments == 1)
			return create(a, getRectangle(a).transformedBy(AffineTransform::scale(getDoubleArgs(a, 0))));

		return var(a.thisObject);
	});

	ADD_VOID_FUNCTION2(setPosition, "(double x, double y)");
	ADD_VOID_FUNCTION2(setSize, "(double width, double height)");
	ADD_VOID_FUNCTION2(setCentre, "(double centerX, double centerY)");
	
	ADD_FUNCTION(setSize, "(double newWidth, double newHeight)", [](const Args& a)
	{
		getRectangle(a).setSize(getDoubleArgs(a, 0), getDoubleArgs(a, 1));
		return var();
	});

	ADD_FUNCTION(constrainedWithin, "(var outerBounds)", [](const Args& a)
	{
		Rectangle<double> r;
		if(getRectangleArgs(a, r))
			return create(a, getRectangle(a).constrainedWithin(r));

		return var(a.thisObject);
	});

	ADD_FUNCTION(contains, "(var otherRectOrPoint)", [](const Args& a)
	{
		Rectangle<double> r;
		Point<double> p;

		if(getRectangleArgs(a, r))
			return getRectangle(a).contains(r);

		if(getPointArgs(a, p))
			return getRectangle(a).contains(p);

		return false;
	});

	ADD_FUNCTION(intersects, "(var otherRect)", [](const Args& a)
	{
		Rectangle<double> r;
		Point<double> p;

		if(getRectangleArgs(a, r))
			return getRectangle(a).intersects(r);

		return false;
	});

	ADD_FUNCTION(getUnion, "(var otherRect)", [](const Args& a)
	{
		Rectangle<double> r;
		if(getRectangleArgs(a, r))
			return create(a, getRectangle(a).getUnion(r));

		return var(a.thisObject);
	});

	ADD_FUNCTION(getIntersection, "(var otherRect)", [](const Args& a)
	{
		Rectangle<double> r;
		if(getRectangleArgs(a, r))
		{
			getRectangle(a).intersectRectangle(r);
			return create(a, r);
		}

		return var(a.thisObject);
	});

	ADD_FUNCTION(toString, "()", [](const Args& a) { return getObject(a)->toString(); });
	ADD_FUNCTION(isEmpty, "()", [](const Args& a) { return getRectangle(a).isEmpty(); });
	
	
	ADD_FUNCTION(reduced, "(double x, double optionalY)", [](const Args& a)
	{
		if(a.numArguments == 1)
			return create(a, getRectangle(a).reduced(getDoubleArgs(a, 0)));
		if(a.numArguments == 2)
			return create(a, getRectangle(a).reduced(getDoubleArgs(a, 0), getDoubleArgs(a, 1)));

		return create(a, getRectangle(a));
	});

	ADD_FUNCTION(expanded, "(double x, double optionalY)", [](const Args& a)
	{
		if(a.numArguments == 1)
			return create(a, getRectangle(a).expanded(getDoubleArgs(a, 0)));
		if(a.numArguments == 2)
			return create(a, getRectangle(a).expanded(getDoubleArgs(a, 0), getDoubleArgs(a, 1)));

		return create(a, getRectangle(a));
	});

	ADD_FUNCTION(withAspectRatioLike, "(var otherRect)", [](const Args& a)
	{
		auto r = getRectangle(a);
		Rectangle<double> other;

		if(getRectangleArgs(a, other))
		{
			auto ar = other.getHeight() / other.getWidth();
			
			auto w = r.getWidth();
			auto h = r.getWidth() * ar;

			float x; float y;
			
			if(ar > 1.0)
			{
				w = r.getHeight() / ar;
				h = r.getHeight();
				x = r.getX() + std::abs(w - r.getWidth()) / 2.0;
				y = r.getY();
			}
			else
			{
				w = r.getWidth();
				h = r.getWidth() * ar;
				x = r.getX();
				y = r.getY() + std::abs(h - r.getHeight()) / 2.0;
			}
			
			return create(a, {x, y, w, h});
		}

		return create(a, r);
	});

}

var RectangleDynamicObject::FunctionMap::invoke(const Identifier& id, const Args& a)
{
	if(contains(id))
		return functions.at(id)(a);
	else
	{
		// not found...
		jassertfalse;
		return {};
	}
}

var RectangleDynamicObject::FunctionMap::create(const Args& a, Rectangle<double> s)
{
	auto copy = getObject(a)->clone();
	auto x = dynamic_cast<RectangleDynamicObject*>(copy.get());
	x->rectangle = s;
	return var(x);
}

bool RectangleDynamicObject::FunctionMap::getRectangleArgs(const Args& a, Rectangle<double>& r)
{
	if(a.numArguments == 4)
	{
		r = Rectangle<double>(a.arguments[0], a.arguments[1], a.arguments[2], a.arguments[3]);
		return true;
	}
	if(a.numArguments == 1)
	{
		if(auto obj = dynamic_cast<RectangleDynamicObject*>(a.arguments[0].getDynamicObject()))
		{
			r = obj->getRectangle();
			return true;
		}
		if(a.arguments[0].isArray() && a.arguments[0].size() == 4)
		{
			r = Rectangle<double>(a.arguments[0][0], a.arguments[0][1], a.arguments[0][2], a.arguments[0][3]);
			return true;
		}
	}

	return false;
}

bool RectangleDynamicObject::FunctionMap::getPointArgs(const Args& a, Point<double>& p)
{
	if(a.numArguments >= 2)
	{
		p = Point<double>(a.arguments[0], a.arguments[1]);
		return true;
	}
	if(a.numArguments == 1)
	{
		if(a.arguments[0].isArray() && a.arguments[0].size() == 2)
		{
			p = Point<double>(a.arguments[0][0], a.arguments[0][1]);;
			return true;
		}
	}

	return false;
}

double RectangleDynamicObject::FunctionMap::getDoubleArgs(const var::NativeFunctionArgs& a, int index,
                                                          double defaultValue)
{
	if(isPositiveAndBelow(index, a.numArguments))
		return (double)a.arguments[index];

	return defaultValue;
}

RectangleDynamicObject* RectangleDynamicObject::FunctionMap::getObject(const var::NativeFunctionArgs& a)
{
	return dynamic_cast<RectangleDynamicObject*>(a.thisObject.getDynamicObject());
}

Rectangle<double>& RectangleDynamicObject::FunctionMap::getRectangle(const var::NativeFunctionArgs& a)
{
	return getObject(a)->rectangle;
}

bool RectangleDynamicObject::hasProperty(const Identifier& propertyName) const
{
	static const std::array<Identifier, 4> ids = { "x", "y", "width", "height" };
	return std::find(ids.begin(), ids.end(), propertyName) != ids.end();
}

const var& RectangleDynamicObject::getProperty(const Identifier& propertyName) const
{
	static const std::array<Identifier, 4> ids = { "x", "y", "width", "height" };

	static var v;

	if(propertyName == ids[0]) v = rectangle.getX();
	if(propertyName == ids[1]) v = rectangle.getY();
	if(propertyName == ids[2]) v = rectangle.getWidth();
	if(propertyName == ids[3]) v = rectangle.getHeight();

	return v;
}

void RectangleDynamicObject::setProperty(const Identifier& propertyName, const var& newValue)
{
	static const std::array<Identifier, 4> ids = { "x", "y", "width", "height" };

	if(propertyName == ids[0]) rectangle.setX((double)newValue);
	if(propertyName == ids[1]) rectangle.setY((double)newValue);
	if(propertyName == ids[2]) rectangle.setWidth((double)newValue);
	if(propertyName == ids[3]) rectangle.setHeight((double)newValue);
}

bool RectangleDynamicObject::hasMethod(const Identifier& methodName) const
{
	return functionMap->contains(methodName);
}

var RectangleDynamicObject::invokeMethod(Identifier methodName, const var::NativeFunctionArgs& args)
{
	return functionMap->invoke(methodName, args);
}

DynamicObject::Ptr RectangleDynamicObject::clone()
{
	jassertfalse;
	return this;
}

void RectangleDynamicObject::writeAsJSON(OutputStream& mos, int indentLevel, bool cond, int i)
{
	auto s = toString();
	mos.writeText(s, false, false, NewLine::getDefault());
}
}
