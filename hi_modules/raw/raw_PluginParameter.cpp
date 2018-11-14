
namespace hise {
using namespace juce;

namespace raw {

float PluginParameter::getValue() const
{
	if (connections.size() == 1)
	{
		auto c = connections.getFirst();
		return jlimit<float>(0.0f, 1.0f, c.range.convertTo0to1(c.processor->getAttribute(c.index)));
	}

	// Not yet implemented
	jassertfalse;
	return 0.0f;
}

void PluginParameter::setValue(float newValue)
{
	if (connections.size() == 1)
	{
		bool *enableUpdate = &getMainController()->getPluginParameterUpdateState();

		if (enableUpdate)
		{
			auto c = connections.getFirst();

			ScopedValueSetter<bool> setter(*enableUpdate, false, true);

			const float convertedValue = c.range.convertFrom0to1(newValue);
			const float snappedValue = c.range.snapToLegalValue(convertedValue);

			if (!lastValueInitialised || lastValue != snappedValue)
			{
				lastValue = snappedValue;
				lastValueInitialised = true;

				c.processor->setAttribute(c.index, snappedValue, sendNotification);
			}
		}

		return;
	}

	// Not yet implemented
	jassertfalse;
}

float PluginParameter::getDefaultValue() const
{
	if (connections.size() == 1)
	{
		auto c = connections.getFirst();
		const float v = c.range.convertTo0to1(c.processor->getDefaultValue(c.index));
		return jlimit<float>(0.0f, 1.0f, v);
	}
	else
	{
		return 0.0f;
	}
}



String PluginParameter::getLabel() const
{
	if (type == Type::Slider)
	{
		return suffix;
	}
	else return String();
}

String PluginParameter::getText(float value, int) const
{
	auto rangeToUse = getRangeToUse();

	switch (type)
	{
	case PluginParameter::Type::Slider:

		return String(rangeToUse.convertFrom0to1(jlimit(0.0f, 1.0f, value)), 1);
		break;
	case PluginParameter::Type::Button:
		return value > 0.5f ? "On" : "Off";
		break;
	case PluginParameter::Type::ComboBox:
	{
		const int index = jlimit<int>(0, itemList.size() - 1, (int)(value*(float)itemList.size()));
		return itemList[index];
		break;
	}
	case PluginParameter::Type::Unsupported:
	default:
		jassertfalse;
		break;
	}

	return String();
}

float PluginParameter::getValueForText(const String &text) const
{
	switch (type)
	{
	case PluginParameter::Type::Slider:
		return text.getFloatValue();
		break;
	case PluginParameter::Type::Button:
		return text == "On" ? 1.0f : 0.0f;
		break;
	case PluginParameter::Type::ComboBox:
		return (float)itemList.indexOf(text);
		break;
	case PluginParameter::Type::Unsupported:
	default:
		break;
	}

	return 0.0f;
}

int PluginParameter::getNumSteps() const
{
	auto rangeToUse = getRangeToUse();

	switch (type)
	{
	case PluginParameter::Type::Slider:
		return (int)((float)rangeToUse.getRange().getLength() / rangeToUse.interval);
	case PluginParameter::Type::Button:
		return 2;
	case PluginParameter::Type::ComboBox:
		return itemList.size();
	case PluginParameter::Type::Unsupported:
		break;
	default:
		break;
	}

	jassertfalse;
	return 2;
}

bool PluginParameter::isMetaParameter() const
{
	return false;
}

void PluginParameter::setParameterNotifyingHost(int index, float newValue)
{
	ScopedValueSetter<bool> setter(getMainController()->getPluginParameterUpdateState(), false, true);

	auto rangeToUse = getRangeToUse();
	auto sanitizedValue = jlimit<float>(rangeToUse.start, rangeToUse.end, newValue);
	auto parentProcessor = dynamic_cast<AudioProcessor*>(getMainController());

	parentProcessor->setParameterNotifyingHost(index, rangeToUse.convertTo0to1(sanitizedValue));
}

}
}