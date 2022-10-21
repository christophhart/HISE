#ifndef __FAUST_STATIC_WRAPPER_H
#define __FAUST_STATIC_WRAPPER_H

namespace scriptnode {
namespace faust{

template <int NV, class FaustClass, class MC, int nChannels> struct faust_static_wrapper: public data::base, public faust_base_wrapper<NV>
{
	// Metadata Definitions ------------------------------------------------------

	SNEX_NODE(faust_static_wrapper);

	using MetadataClass = MC;

	// set to true if you want this node to have a modulation dragger
	static constexpr bool isModNode() { return false; };
	static constexpr bool isPolyphonic() { return NV > 1; };
	// set to true if your node produces a tail
	static constexpr bool hasTail() { return false; };

	// Define the amount and types of external data slots you want to use
	static constexpr int NumTables = 0;
	static constexpr int NumSliderPacks = 0;
	static constexpr int NumAudioFiles = 0;
	static constexpr int NumFilters = 0;
	static constexpr int NumDisplayBuffers = 0;

	PolyData<FaustClass, NV> faust_obj;

	faust_static_wrapper():
		faust_base_wrapper<NV>()
	{
		auto basePointer = this->faustDsp.begin();
		auto objPointer = faust_obj.begin();

		for (int i = 0; i < NV; i++)
		{
			basePointer[i] = &objPointer[i];
			objPointer[i].buildUserInterface(&this->ui);
		}
	}

	// Scriptnode Callbacks ------------------------------------------------------

	void prepare(PrepareSpecs ps) override
	{
		faust_obj.prepare(ps);

		faust_base_wrapper<NV>::prepare(ps);
	}

	template <typename T> void process(T& data)
	{
        
		faust_base_wrapper<NV>::process(data.template as<ProcessDataDyn>());
	}

	template <typename T> void processFrame(T& data)
	{

	}

	void setExternalData(const ExternalData& data, int index)
	{

	}
	// Parameter Functions -------------------------------------------------------

	template <int P> void setParameter(double v)
	{
		jassert(P < this->ui.parameters.size());

		for (auto z : this->ui.parameters[P]->zone)
			*z = (float)v;
	}

	void createParameters(ParameterDataList& data)
	{
		int i = 0;
		for (const auto& p : this->ui.parameters)
		{
			auto pd = p->toParameterData();

			pd.callback.referTo(p.get(), faust_ui<NV>::Parameter::setParameter);

			pd.info.index = i++;
			data.add(std::move(pd));
		}
	}

	// provide compile-time constant number of channels (e.g., to HardcodedMasterFX)
	static constexpr int getFixChannelAmount() {
		return nChannels;
	}
};
}
}

#endif // __FAUST_STATIC_WRAPPER_H

#if 0
// generated code
using Meta = faust::Meta;
using UI = faust::UI;
#include "src/stereo-delay.cpp"
namespace project {
struct StereoDelayMetaData {
		SN_NODE_ID("faust_mockup");
};
template <int NV>
using faust_mockup = scriptnode::faust::faust_static_wrapper<1, _stereo_delay, StereoDelayMetaData>;
} // namespace project


#endif // 0
