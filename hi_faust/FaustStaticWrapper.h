#pragma once

namespace scriptnode {
namespace faust{

template <int NV, class ModParameterClass, class FaustClass, class MC, int nChannels> struct faust_static_wrapper: public data::base, public faust_base_wrapper<NV, ModParameterClass>
{
	// Metadata Definitions ------------------------------------------------------

	SNEX_NODE(faust_static_wrapper);

	using MetadataClass = MC;

	// set to true if you want this node to have a modulation dragger
	static constexpr bool isModNode() { return false; };
	static constexpr bool isPolyphonic() { return NV > 1; };
	// set to true if your node produces a tail
	static constexpr bool hasTail() { return false; };

	static constexpr bool isSuspendedOnSilence() { return false; };

	// Define the amount and types of external data slots you want to use
	static constexpr int NumTables = 0;
	static constexpr int NumSliderPacks = 0;
	static constexpr int NumAudioFiles = 0;
	static constexpr int NumFilters = 0;
	static constexpr int NumDisplayBuffers = 0;

	PolyData<FaustClass, NV> faust_obj;

    auto& getParameter()
    {
        return this->ui.modParameters;
    }
    
	faust_static_wrapper():
		faust_base_wrapper<NV, ModParameterClass>()
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

		faust_base_wrapper<NV, ModParameterClass>::prepare(ps);
	}

	template <typename T> void process(T& data)
	{
		faust_base_wrapper<NV, ModParameterClass>::process(data.template as<ProcessDataDyn>());
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

			pd.callback.referTo(p.get(), faust_ui<NV, ModParameterClass>::Parameter::setParameter);

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


