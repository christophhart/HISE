


class ConvertedTccScriptFactory : public StaticDspFactory
{
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("tcc") };

	void registerModules() override;
};

