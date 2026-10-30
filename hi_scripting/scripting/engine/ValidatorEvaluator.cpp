
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

namespace hise {
using namespace juce;

struct LightweightDiagnostics::ValidatorEvaluator
{
	ValidatorEvaluator(LightweightDiagnostics* lb_, const Identifier& methodName_, const Array<var>& values, int index) :
		lb(lb_),
		methodName(methodName_),
		parameterIndex(index),
		thisValue(values[index])
	{
		for (auto& f : lb->functions)
		{
			if (f.methodName == methodName)
			{
				argumentId = index == -1 ? "this" : f.arguments[index].id;

				for (int i = 0; i < values.size(); i++)
				{
					if (!values[i].isUndefined())
						resolvedArguments.set(f.arguments[i].id, values[i]);
				}
			}
		}
	}

	DiagnosticResult performCallbackCheck(const String& functionPrototype)
	{
		auto cd = makeCustomCheckData(functionPrototype);

		auto customResult = lb->performCustomCheck(cd);

		if (customResult.getSeverity() == DiagnosticResult::Severity::Unknown)
		{
			auto f = cd.resolvedArguments[cd.parameterId];

			CallbackParseOptions cpo;

			cpo.callback = f;
			cpo.thisObj = lb;

			if (functionPrototype.startsWith("Realtime?"))
			{
				auto conditionVar = functionPrototype.fromFirstOccurrenceOf("Realtime?", false, false)
					.upToFirstOccurrenceOf("(", false, false)
					.trim();

				auto syncValue = resolvedArguments[Identifier(conditionVar)];

				// If sync == Synchronous, check that it's realtime capable...
				auto notification = ApiHelpers::getDispatchType(syncValue, false);

				if (notification == dispatch::DispatchType::sendNotificationSync)
				{
					cd.checkString = "Realtime" + functionPrototype.fromFirstOccurrenceOf("(", true, false);
				}
				else if (notification == dispatch::DispatchType::sendNotificationAsync)
				{
					cd.checkString = "Function" + functionPrototype.fromFirstOccurrenceOf("(", true, false);
				}
			}

			auto ok = cpo.parseCheckString(cd.checkString, true);

			if (!ok.wasOk())
				return DiagnosticResult::fail(ok.getErrorMessage()).withWrongToken(cd.parameterId.toString());

			customResult = lb->shadowParseCallbackWithArgType(cpo);
		}

		return customResult;
	}

	DiagnosticResult performStringValidation(const String& checkType)
	{
		auto valueToCheck = thisValue.toString();

		if (checkType == "ValidVariableId")
		{
			auto ok = Identifier::isValidIdentifier(valueToCheck);

			if (!ok)
			{
				auto correct = valueToCheck.removeCharacters(" +/()\t.\"");
				return DiagnosticResult::fail("must be a valid identifier string").withExpectation(correct, valueToCheck);
			}


			return DiagnosticResult::ok();
		}
		if (checkType.startsWith("Option"))
		{
			if (checkType.contains("$CUSTOM"))
			{
				auto customType = checkType.fromFirstOccurrenceOf("$CUSTOM", false, false).upToLastOccurrenceOf("$]", false, false);

				return lb->performCustomCheck(makeCustomCheckData(customType));
			}

			auto s = JSON::parse(checkType.fromFirstOccurrenceOf("Option", false, false));

			StringArray options;

			if (auto ar = s.getArray())
			{
				for (auto& v : *ar)
					options.add(v.toString());
			}

			if (!options.contains(valueToCheck))
				return DiagnosticResult::fail("illegal string value")
				.withWrongToken(valueToCheck)
				.withFuzzySuggestion(options);

			return DiagnosticResult::ok();
		}

		return DiagnosticResult::fail("unimplemented: " + checkType);

	}

	DiagnosticResult performRuntimeIdCheck(const String& idType)
	{
		if (idType == "ScriptComponent")
		{
			auto c = lb->p->getScriptingContent();
			auto sc = c->getComponentWithName(thisValue.toString());

			if (sc != nullptr)
				return DiagnosticResult::ok();

			StringArray ids;

			for (int i = 0; i < c->getNumComponents(); i++)
				ids.add(c->getComponent(i)->getId());

			return DiagnosticResult::fail("Component not found")
				.withWrongToken(thisValue.toString())
				.withFuzzySuggestion(ids);
		}

		auto ok = lb->performCustomCheck(makeCustomCheckData(idType));

		if (ok.getSeverity() == DiagnosticResult::Severity::Unknown)
			return DiagnosticResult::fail("runtime check for " + idType + " not implemented yet");

		return ok;
	}

	DiagnosticResult performObjectTypeCheck(const String& objectTypeToCheck)
	{
		auto t = Helpers::getTypeForObjectTypeCheck(thisValue);

		if (t != objectTypeToCheck)
			return DiagnosticResult::fail("wrong type for parameter " + argumentId.toString())
			.withExpectation(objectTypeToCheck, t);

		return DiagnosticResult::ok();
	};

	DiagnosticResult performMethodTouchedCheck(const String& methodName)
	{
		if (methodName == "$Reset$")
		{
			lb->clearTouchedMethods();
			return DiagnosticResult::ok();
		}

		auto sa = StringArray::fromTokens(methodName, ",", "");
		sa.trim();
		sa.removeEmptyStrings();

		Array<Identifier> methods;

		for (const auto& s : sa)
			methods.add(s);

		return checkMethodsTouched(methods, "Must be called after another function");
	}

	DiagnosticResult evaluate(const String& validationString)
	{
		auto cat = validationString.upToFirstOccurrenceOf(".", false, false);
		auto v = validationString.fromFirstOccurrenceOf(".", false, false);

		String overrideMessage;
		String overrideSuggestion;
		DiagnosticResult::Severity overrideSeverity = DiagnosticResult::Severity::Unknown;

		if (v.contains("$?"))
		{
			auto overrideDiagnostic = v.fromFirstOccurrenceOf("$?", false, false);

			auto sevString = overrideDiagnostic.upToFirstOccurrenceOf("$:", false, false);

			for (int i = 0; i < (int)DiagnosticResult::Severity::numSeverity; i++)
			{
				auto si = (DiagnosticResult::Severity)i;
				if (sevString == DiagnosticResult::getSeverityString(si))
				{
					overrideSeverity = si;
					break;
				}
			}

			overrideMessage = overrideDiagnostic.fromFirstOccurrenceOf(":", false, false);

			if (overrideMessage.contains("$>"))
			{
				overrideSuggestion = overrideMessage.fromFirstOccurrenceOf("$>", false, false);
				overrideMessage = overrideMessage.upToFirstOccurrenceOf("$>", false, false);
			}

			v = v.upToFirstOccurrenceOf("$?", false, false);
		}

		using EC = DiagnosticBase::DiagnosticResult::ErrorCode;

		auto wrapDiagnostic = [&](DiagnosticResult r, DiagnosticBase::DiagnosticResult::ErrorCode ec)
		{
			if (!r.shouldReport())
				return r;

			if (overrideSeverity != DiagnosticResult::Severity::Unknown)
				r = r.withSeverity(overrideSeverity);

			if (overrideMessage.isNotEmpty())
				r = r.withMessage(overrideMessage);

			if (overrideSuggestion.isNotEmpty())
				r = r.withSuggestion(overrideSuggestion, true);

			if(ec != EC::Unknown)
				r = r.withErrorCode(ec);

			return r;
		};

		c = (Category)getCategories().indexOf(cat);

		switch (c)
		{
		case Category::RangeCheck:			return wrapDiagnostic(performRangeCheck(v), EC::RangeCheck);
		case Category::GraphicsCheck:		return wrapDiagnostic(performGraphicsCheck(v), EC::GraphicsIssue);
		case Category::StringValidator:		return wrapDiagnostic(performStringValidation(v), EC::InvalidOption);
		case Category::Conditional:			return wrapDiagnostic(lb->performCustomCheck(makeCustomCheckData(v)), EC::Unknown);
		case Category::RuntimeIdCheck:		return wrapDiagnostic(performRuntimeIdCheck(v), EC::RuntimeIdFailure);
		case Category::CallbackArgCheck:	return wrapDiagnostic(performCallbackCheck(v), EC::CallbackArgMismatch);
		case Category::ObjectType:			return wrapDiagnostic(performObjectTypeCheck(v), EC::TypeMismatch);
		case Category::MethodTouched:		return wrapDiagnostic(performMethodTouchedCheck(v), EC::UntouchedMethod);
		default:							return DiagnosticResult::fail("unimplemented category " + cat).withErrorCode(EC::InternalError);
		}

		return DiagnosticResult::unknown();
	}

private:

	CustomCheckData makeCustomCheckData(const String& checkString) const
	{
		CustomCheckData cd;
		cd.category = c;
		cd.methodName = methodName;
		cd.parameterId = argumentId;
		cd.resolvedArguments = resolvedArguments;
		cd.checkString = checkString;

		return cd;
	}

	DiagnosticResult performRangeCheck(const String& valueString)
	{
		var result;
		auto ok = JSON::parse(valueString, result);

		if (!ok.wasOk())
			return DiagnosticResult::fail(ok.getErrorMessage());;

		auto inclusive = (bool)result["inclusive"];

		auto minDefined = false;
		auto maxDefined = false;
		auto minOk = true;
		auto maxOk = true;

		auto clamped = (bool)result["clamps"];

		if (result.hasProperty("min"))
		{
			minDefined = true;
			minOk = (thisValue > result["min"]) || (inclusive && result["min"] == thisValue);
		}


		if (result.hasProperty("max"))
		{
			maxDefined = true;
			maxOk = (thisValue < result["max"]) || (inclusive && result["max"] == thisValue);
		}

		String message;

		if (!minOk || !maxOk)
		{
			if (minDefined && maxDefined)
				message << "outside of parameter range [" << result["min"].toString() << ", " << result["max"].toString() << "]";

			else if (minDefined)
				message << "below minimum value " << result["min"].toString();
			else
				message << "above maximum value " << result["min"].toString();

			if (clamped)
				message << " (internally clamped to valid range)";

			return createWarning(message);
		}

		return DiagnosticResult::ok();
	}

	DiagnosticResult createHint(const String& warningMessage)
	{
		return createError(warningMessage).withSeverity(DiagnosticResult::Severity::Hint);
	}

	DiagnosticResult createWarning(const String& warningMessage)
	{
		return createError(warningMessage).withSeverity(DiagnosticResult::Severity::Warning);
	}

	DiagnosticResult createError(const String& errorMessage)
	{
		String msg;

		if (parameterIndex != -1)
			msg << "argument " << String(parameterIndex + 1) << "(" + argumentId + "): ";

		msg << errorMessage;

		return DiagnosticResult(msg)
			.withWrongToken(thisValue.toString())
			.withSeverity(DiagnosticResult::Severity::Error);
	}

#if 0
	static DR checkColourSet(DiagnosticBase* c, const Identifier& id, const Array<var>& args)
	{
		auto colourSet = c->wasMethodTouched("setColour");
		auto gradientSet = c->wasMethodTouched("setGradientFill");

		if (!colourSet && !gradientSet)
			return DR("called before setColour / setGradientFill").withSeverity(DR::Severity::Warning);

		return DR::ok();
	}

	static DR checkInLayer(DiagnosticBase* c, const Identifier& id, const Array<var>& args)
	{
		if (!c->wasMethodTouched("beginLayer"))
			return DR::fail("no graphic layer set").withSuggestion("Call g.beginLayer(...) before using this method");

		return DR::ok();
	}

	static DR checkFontSet(DiagnosticBase* c, const Identifier& id, const Array<var>& args)
	{
		auto f1 = c->wasMethodTouched("setFont");
		auto f2 = c->wasMethodTouched("setFontWithSpacing");

		if (!f1 && !f2)
			return DR("font not set").withSeverity(DR::Severity::Warning);

		return DR::ok();
	}

	static DR checkAreaAsArray(DiagnosticBase* c, const Identifier& id, const Array<var>& args)
	{
		int numExpected = 0;
		int unused = 0;
		c->getIndexAndNumArgsForFunction(id, unused, numExpected);

		if (args.size() > numExpected)
		{
			String sug;
			sug << "g." << id.toString() << "([x, y, w, h]";

			if (numExpected > 1)
				sug << ", ...";

			sug << ")";

			return DR::fail("wrong signature").withSuggestion(sug);
		}

		return DR::ok();
	}
#endif

	DiagnosticResult checkMethodsTouched(const Array<Identifier>& requiredMethods, const String& errorMessage)
	{
		auto ok = false;

		for (const auto& m : requiredMethods)
		{
			ok |= lb->wasMethodTouched(m);

			if (ok)
				break;
		}

		if (ok)
			return DiagnosticResult::ok();

		auto dr = createWarning(errorMessage);

		for (const auto& m : requiredMethods)
			dr = dr.withSuggestion(lb->getObjectName() + "." + m.toString() + "()");

		return dr;
	}

	DiagnosticResult performGraphicsCheck(const String& valueString)
	{
		if (valueString == "ValidColour")
			return DiagnosticResult::ok();

		if (valueString == "ColourSet")
			return checkMethodsTouched({ "setColour", "setGradientFill" }, "Colour not set");

		if (valueString == "FontSet")
		{
			return checkMethodsTouched({ "setFont", "setFontWithSpacing" }, "Font not set");
		}

		if (valueString == "ValidJustification")
		{
			Result re = Result::ok();
			auto just = ApiHelpers::getJustification(thisValue.toString(), &re);

			if (re.failed())
				return createError(re.getErrorMessage()).withFuzzySuggestion(ApiHelpers::getJustificationNames());

			return DiagnosticResult::ok();
		}

        if(valueString == "Point")
        {
            auto r = Result::ok();

            if (auto lb = dynamic_cast<LightweightDiagnostics*>(thisValue.getObject()))
            {
                auto classId = lb->getObjectName().toString();

                if (classId == "Array")
                    return DiagnosticResult::ok();
            }

            auto point = ApiHelpers::getPointFromVar(thisValue, &r);

            if (r.failed())
                return DiagnosticResult::fail(r.getErrorMessage());

            return DiagnosticResult::ok();
        }
        
		if (valueString == "Area")
		{
			auto r = Result::ok();

			if (auto lb = dynamic_cast<LightweightDiagnostics*>(thisValue.getObject()))
			{
				auto classId = lb->getObjectName().toString();

				if (classId == "Array" || classId == "Rectangle")
					return DiagnosticResult::ok();

				return DiagnosticResult::fail("area parameter needs a Rectangle or Array").withWrongToken(classId);
			}

			auto rect = ApiHelpers::getRectangleFromVar(thisValue, &r);

			if (r.failed())
				return DiagnosticResult::fail(r.getErrorMessage());

			return DiagnosticResult::ok();
		}

		return createError("not implemented");
	}

	Category c;
	int parameterIndex;
	var thisValue;
	Identifier argumentId;
	LightweightDiagnostics* lb;
	Identifier methodName;
	NamedValueSet resolvedArguments;
};

DiagnosticBase::DiagnosticResult LightweightDiagnostics::Validator::perform(DiagnosticBase* b, const Identifier& id, const Array<var>& args) const
{
	auto ok = DiagnosticResult::ok();

	for (const auto& v : validators)
	{
		if (v.first == -1)
		{
			ValidatorEvaluator p(dynamic_cast<LightweightDiagnostics*>(b), id, args, -1);

			for (const auto& s : v.second)
				ok = DiagnosticResult::max(ok, p.evaluate(s));
		}

		if (isPositiveAndBelow(v.first, args.size()))
		{
			auto value = args[v.first];

			if (!value.isUndefined())
			{
				ValidatorEvaluator p(dynamic_cast<LightweightDiagnostics*>(b), id, args, v.first);

				for (const auto& s : v.second)
					ok = DiagnosticResult::max(ok, p.evaluate(s));
			}
		}
	}

	return ok;
}

}