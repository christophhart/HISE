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

namespace scriptnode {
using namespace juce;
using namespace hise;

#if 0
namespace meta
{

namespace width_bandpass_impl
{

using width_bandpass_ = container::chain<2, filters::svf, filters::svf>;

DSP_METHODS_PIMPL_IMPL(width_bandpass_);

void instance::createParameters(ParameterDataList& data)
{
	auto& obj = *pimpl;

	// Node Registration ===============================================================
	registerNode(FIND_NODE(obj, 0), "hp");
	registerNode(FIND_NODE(obj, 1), "lp");

	// Parameter Initalisation =========================================================
	setParameterDefault("hp.Frequency", 20000.0);
	setParameterDefault("hp.Q", 1.0);
	setParameterDefault("hp.Gain", 0.0);
	setParameterDefault("hp.Smoothing", 0.00873185);
	setParameterDefault("hp.Mode", 1.0);
	setParameterDefault("lp.Frequency", 20000.0);
	setParameterDefault("lp.Q", 1.0);
	setParameterDefault("lp.Gain", 0.0);
	setParameterDefault("lp.Smoothing", 0.00873185);
	setParameterDefault("lp.Mode", 0.0);

	// Setting node properties =========================================================

	// Parameter Callbacks =============================================================
	{
		ParameterData p("Frequency", { 20.0, 20000.0, 0.1, 1.0 });
		p.setDefaultValue(20000.0);

		auto param_target1 = getCombinedParameter("hp.Frequency", { 20.0, 20000.0, 0.1, 0.229905 }, "SetValue");
		auto param_target2 = getCombinedParameter("lp.Frequency", { 20.0, 20000.0, 0.1, 0.229905 }, "SetValue");

		p.setCallback([param_target1, param_target2, outer = p.range](double newValue)
		{
			auto normalised = outer.convertTo0to1(newValue);
			param_target1->SetValue(normalised);
			param_target2->SetValue(normalised);
		});
		data.add(std::move(p));
	}
	{
		ParameterData p("Width", { 0.0, 1.0, 0.01, 1.0 });

		auto param_target1 = getCombinedParameter("hp.Frequency", { 0.0, 1.0, 0.1, 0.229905 }, "Multiply");
		auto param_target2 = getCombinedParameter("lp.Frequency", { 1.0, 16.0, 0.1, 0.229905 }, "Multiply");
		param_target2->addConversion(ConverterIds::SubtractFromOne, "Multiply");
		auto param_target3 = getParameter("hp.Q", { 1.0, 4.0, 0.1, 1.0 });
		auto param_target4 = getParameter("lp.Q", { 1.0, 4.0, 0.1, 1.0 });

		p.setCallback([param_target1, param_target2, param_target3, param_target4](double newValue)
		{
			param_target1->Multiply(newValue);
			param_target2->Multiply(newValue);
			param_target3(newValue);
			param_target4(newValue);
		});
		data.add(std::move(p));
	}
	{
		ParameterData p("Smoothing", { 0.0, 1.0, 0.01, 1.0 });
		p.setDefaultValue(0.24);

		auto param_target1 = getParameter("hp.Smoothing", { 0.0, 1.0, 0.01, 0.30103 });
		auto param_target2 = getParameter("lp.Smoothing", { 0.0, 1.0, 0.01, 0.30103 });

		p.setCallback([param_target1, param_target2](double newValue)
		{
			param_target1(newValue);
			param_target2(newValue);
		});
		data.add(std::move(p));
	}
}

String instance::getSnippetText() const
{
	return "737.3oc6V86aSCDE94ld7ihPJUvbGfUTTZ.DpSNhT0pH0j1hKvH5Z705S01mw4bCtvBBPBI9K.FfUlXhgxV+CfhPp+E.aHVBKLvBbmSSi+ET21zBEkawwWd268887688t5LcBnLR0IAk7nVTctwcVDaq6fa1DlB2fyb8mCyM.kQQMX1bL0l3Vngg3IT2yphA11lX1DTPJCA.LCqwJQ1VIWcQ.pvLYdtRiD1.UXVVDaNnLDp6uusLvx++RBKttuL52.a4r.0hTS3kSOLDrtRYMCVq4vtXKBm3J8+PcrlnKiUmn0DDttd.uNkjWCiLbhxkyhVhZJcPglqtzeEdnjqFS2yDyoL6EvtKS3BTCgXlvI67l3fUsWk3x6vxZT6agM8Hg8WM78ht2S2p7LrVD2YnVTdXKuoiS7cE1pwINZz0BN+Kdtb8NUsUHs5j1j6t4C9x0dj0lpxL5YPS4RtqGwtgOjHrPVv8kCVaoFF2a9d45hwvcWKih6t1t639Gu70e+Be6iA3NGZ9X3EZqBYOOWZiD4YnTp44RajRdVX6ti2tvRh2SflV1nEKhgKT1UHCPRH2VMMHCPZPtsZXHe+y+lo+zZONFj+o+ql6bSt01kFZVLF2fZubOb+r2NVQkO+P0CHtyWN63NejT8Nb4OjpqIULRo5vkI7NmJkUfHpJlCTUFnpLPUYfpxdUUQlpCqpDoAMWH1fh2Vj0lzdnDgJVX7jsSJJUXBQnFRkhf6K06UQDkpRU029lS6fG4NgAi3HA4ZAPGAUUWHTQ49vrNK36HuRIRiv6D2+cjYfeCMM+OilwqgNIpyLjr1ImV2YhCFuFZ3CgZnZdlbpioOzuzi1uI2QD8QkJMwDEuJb.KhFEo4sH2U32obYVyZS16zssZR5Vr7QLci+YUNFrO0qjF+x2WmfmCMdl+NdrhXHz3EJBPxgH8Ff1WDAVerm704W+Cp6+AI8.TeJ4dTdojrqAb7llQtmxuvJWgaG";
}



REGISTER_MONO;

}

namespace filter_delay_impl
{

// Template Alias Definition =======================================================
using frame2_block2_ = container::frame2_block<0, routing::receive, meta::width_bandpass, core::fix_delay, routing::send>;
using filter_delay_ = container::chain<0, bypass::yes<routing::matrix>, bypass::yes<routing::matrix>, bypass::yes<routing::matrix>, wrap::mod<core::tempo_sync>, core::gain, frame2_block2_>;

DSP_METHODS_PIMPL_IMPL(filter_delay_);

String instance::getSnippetText() const
{
	return "1579.3oc6X0zbaSDFdUR1jlOZoLzYJ+BXfofFY4j51CfsiMt0M1ocrcKeLcFOxVqsUhjVUYojpxAlggCbqmgqbmgqbD9KvAlggKvufdggqrq9bWI4T2z3PNfujrR6p8884448ic2Gqh.Ba0rNP3JvQZ5NH69pHcEOPCkgNXauGn3LAH7lvgXSGEMSjs3vIj+B120n1DESSj9TfvxPYPK7vCS8PIv9jOeMrN10NXbMrgAxzAHrDL5++DMU5N.gEkj.65YoLcZGECqdZFn1juxJPYIP2I3iefhshAhXe9e5BfF1HzyPpA12aAIuRQjyAB9VH0fcNb5MUy3nBKSsxo.fvp66iFaRQiMg5nQN8wl5ofhqBswtNZliEMTbr0d54KRj3TBKU2yTwPaXvinFlOyTPLbiAMv5pAteAxpaiUc0Ubzvl8TrGibHNLnSfmzNvQDfDqtKwBGh3cnvQRAVdWjopD0b9fBQuoP3P5qh9e.CgQFXisP1NZ93rP3HJ1Gn79XiAHUhwVWwQA7HEcWBMffkjEKhu0t26Fc1qoWoNxx02Ca15Iip13yjb630tXo8lZdPWosa0okaoNsMlT8dMzZcrXsVhG5VcTwZGrWspFetJVT26FU2cz8viOpXsNcJN3SEEu6g0mb.3AtCz0FF3Y.F9+RvoDSGguHQ9uJDNiPY4EN4S2vyTtWCVZmydteTQu61qY655GzRTbjrkzwGNS9eKns13I+eBfTZf3v7yAQvDXoseMEA2.MZzntsc6Mxqp0AM1cx80DeXq6FoAZs8Qia0LOMvknl0kgNHCKb+odlCkyVPzFIl79yWUfvx4vlBBoeHPXc5FRK5sIon2SCq3ESLAunN8gzOOno4QDVIpjYaMyPlvejxSCGsJrfDwzZgOFY2RyPyIXBOzxJdb3T55fr5p8LD0ajDIxiCQGG.iDKi7jhREjJxKTDVJdDMRbtLnUfEt0IZN9SHwX7UpLlBcLkuWC1ixmQ5u0WA3+6cpTCSnvgTbk1pAa2Ch9318s54YQVwFvtHmfUCXbi0msaTf2MtYJ2nPZ23lygarErsqtilktFYyS4KWsBWjHqZeM3XR1iT57MCz4iO2a4KWENeVjkRkEYKXGzzH7mQnvEdmZMuA7gSQyyxdkknqRxAJwpRoulkcCm.KA6Wg8DiY1QbaYY4R977pv6PokTLL.vpVuFuZsookqyqgbcQlFHnq6tFXryDRMIPbTgbbV40BnrQTSUt+.ch9Kkd8sYNnB6zVPh2jyNrBSjzVPUcu9iFzmTcMcYindGrQCQZGgtvETktz7kgUUU6g6pM1TQGvj2Z1gGBu5YvoevSRH4mpKVEsJQyHketuMfMPH0AJDFONt3i1329dqO72KyFWbc93h3EkWnACydU3wTXt+.ESU+lt3X2qCG5N0AaHlZRbzITXIBlmgkCTRQbLTHHPNjk2D1v07PuAXuMRS2BuOYpYo6nrAaWYFbNKgsb9A+jUFSWIeuDRK7YeyuVgk5RlIKAlL2DZb8U9tuk96mJykPHb8unbPBAxA2ehKxbnWBg5urc94Jf42tAfL1MYGxytAfbraxbYs6u3Z+vc9ym80mfcuFLffh2xm+K+8693m+Wku.ay4j7MYClYaCrMVl5bR9sNj71KBo5lUY7M9urlWRC37vdkurR1N5oY5NkUqk29kZ4jo7xavjjjUQEkiA+ieUkSPmDVTjzxBmN4Jw0DmRN83E9BhaAYpiD59bE7mYgwYefXdc4pLr6JzsbcXzI5mE6tFTV71291Izwi8y27OkEDRrV+T8rEA2L4vgIW1I2gC2.FeeCjE5qynAKvlpD.VifI4Tu7zUwmFkrCmbDBKHVHUbzNYzifbcn3auaA5ME37F4En2vcWTKPORlyiJd13QoEyLcncRZiLcukVIKLCnhMRjGph22ErT9T07JKNACNcmeB1rsE7hxYaKPpRly4ymFe9PO9K7hC93tTgSA.lWCN46I40fyKJmWCNRbMSF2zStM3jQGx1Y4bIDeO+bp+wbJDybRgTW5VxleFAlmmclmALiZ2ct.xnVgOK.xnM9LBDOOa4NGPL35YlKPL5qLefXzc6wgcg2ezqc1vE6Mcw2nz+B0P7WfA";
}

void instance::createParameters(ParameterDataList& data)
{
	auto& obj = *pimpl;

	// Node Registration ===============================================================
	registerNode(FIND_NODE(obj, 0), "left_only");
	registerNode(FIND_NODE(obj, 1), "stereo");
	registerNode(FIND_NODE(obj, 2), "right_only");
	registerNode(FIND_NODE(obj, 3), "tempo_sync2");
	registerNode(FIND_NODE(obj, 4), "gain2");
	registerNode(FIND_NODE(obj, 5, 0), "dly_fb_out");
	registerNode(FIND_NODE(obj, 5, 1), "width_bandpass");
	registerNode(FIND_NODE(obj, 5, 2), "fix_delay");
	registerNode(FIND_NODE(obj, 5, 3), "dly_fb_in");

	// Parameter Initalisation =========================================================
	setParameterDefault("tempo_sync2.Tempo", 11.0);
	setParameterDefault("tempo_sync2.Multiplier", 4.0);
	setParameterDefault("gain2.Gain", 0.0);
	setParameterDefault("gain2.Smoothing", 20.0);
	setParameterDefault("dly_fb_out.Feedback", 0.41);
	setParameterDefault("width_bandpass.Frequency", 8811.2);
	setParameterDefault("width_bandpass.Width", 0.58);
	setParameterDefault("width_bandpass.Smoothing", 0.0);
	setParameterDefault("fix_delay.DelayTime", 500.0);
	setParameterDefault("fix_delay.FadeTime", 598.0);

	// Setting node properties =========================================================
	setNodeProperty("left_only.EmbeddedData", "72.3o8BJ+RKIy7R22DKonLqfAFY0uRyM37KsnjS04LRLu7RMmhAJFiLw.CL.kuAf3CjKCAmZdo.ly+ABfJogv3CRR3bX..HkDhj", false);
	setNodeProperty("stereo.EmbeddedData", "75.3o8BJ+RKIy7R22DKonLqfAFY0uRyM37KsnjS04LRLu7RMmhAJFiLw.CL.kuAf3CjKCAmZdo.ly+ABfJogf3yHTIMDljL..f2p0wk", false);
	setNodeProperty("right_only.EmbeddedData", "74.3o8BJ+RKIy7R22DKonLqfAFY0uRyM37KsnjS04LRLu7RMmhAJFiLw.CL.kuAf3+efffSMuTfyApjFBhOi.ULHIMDljL..L4vgLI", false);
	setNodeProperty("gain2.ResetValue", "0", false);
	setNodeProperty("gain2.UseResetValue", "0", false);
	setNodeProperty("dly_fb_out.AddToSignal", "1", false);
	setNodeProperty("dly_fb_in.Connection", "dly_fb_out", false);

	// Internal Modulation =============================================================
	{
		auto mod_target1 = getParameter("fix_delay.DelayTime", { 0.0, 1000.0, 0.1, 0.30103 });
		auto f = [mod_target1](double newValue)
		{
			mod_target1.callUnscaled(newValue);
		};


		setInternalModulationParameter(FIND_NODE(obj, 3), f);
	}

	// Parameter Callbacks =============================================================
	{
		ParameterData p("Channel", { 0.0, 2.999, 0.01, 1.0 });
		p.setDefaultValue(1.46);

		auto bypass_target1 = getParameter("left_only.Bypassed", { 0.0, 1.0, 0.5, 1.0 });
		auto bypass_target2 = getParameter("stereo.Bypassed", { 1.0, 2.0, 0.5, 1.0 });
		auto bypass_target3 = getParameter("right_only.Bypassed", { 2.0, 3.0, 0.5, 1.0 });

		p.setCallback([bypass_target1, bypass_target2, bypass_target3](double newValue)
		{
			bypass_target1.setBypass(newValue);
			bypass_target2.setBypass(newValue);
			bypass_target3.setBypass(newValue);
		});
		data.add(std::move(p));
	}
	{
		ParameterData p("Feedback", { 0.0, 1.0, 0.01, 1.0 });
		p.setDefaultValue(0.41);

		auto param_target1 = getParameter("dly_fb_out.Feedback", { 0.0, 1.0, 0.01, 1.0 });

		p.setCallback([param_target1](double newValue)
		{
			param_target1(newValue);
		});
		data.add(std::move(p));
	}
	{
		ParameterData p("Time", { 1.0, 16.0, 1.0, 1.0 });
		p.setDefaultValue(4.0);

		auto param_target1 = getParameter("tempo_sync2.Multiplier", { 1.0, 16.0, 1.0, 1.0 });

		p.setCallback([param_target1, outer = p.range](double newValue)
		{
			auto normalised = outer.convertTo0to1(newValue);
			param_target1(normalised);
		});
		data.add(std::move(p));
	}
	{
		ParameterData p("Frequency", { 0.0, 1.0, 0.01, 1.0 });
		p.setDefaultValue(0.44);

		auto param_target1 = getParameter("width_bandpass.Frequency", { 20.0, 20000.0, 0.1, 1.0 });

		p.setCallback([param_target1](double newValue)
		{
			param_target1(newValue);
		});
		data.add(std::move(p));
	}
	{
		ParameterData p("Width", { 0.0, 1.0, 0.01, 1.0 });
		p.setDefaultValue(0.58);

		auto param_target1 = getParameter("width_bandpass.Width", { 0.0, 1.0, 0.01, 1.0 });

		p.setCallback([param_target1](double newValue)
		{
			param_target1(newValue);
		});
		data.add(std::move(p));
	}
	{
		ParameterData p("Input", { 0.0, 1.0, 0.01, 1.0 });
		p.setDefaultValue(1.0);

		auto param_target1 = getParameter("gain2.Gain", { -100.0, 0.0, 0.1, 5.42227 });

		p.setCallback([param_target1](double newValue)
		{
			param_target1(newValue);
		});
		data.add(std::move(p));
	}
}



REGISTER_MONO;

}

namespace tremolo_impl
{
// Template Alias Definition =======================================================
using stereo_ctrl_ = container::chain<parameter::empty, math::mul, math::sig2mod, wrap::mod<core::peak>>;
using mono_ctrl_ = container::chain<parameter::empty, math::mul, math::sig2mod, wrap::mod<core::peak>>;
using split_ = container::split<parameter::empty, stereo_ctrl_, mono_ctrl_>;
using modchain_ = container::modchain<parameter::empty, core::oscillator, split_>;
using multi_ = container::multi<parameter::empty, fix<1, core::gain>, fix<1, core::gain>>;
using tremolo_ = container::chain<parameter::empty, math::mul, modchain_, core::gain, multi_>;

DSP_METHODS_PIMPL_IMPL(tremolo_);

juce::String instance::getSnippetText() const
{
	return "1151.3oc6Y06aaaDE+NJewV1NINItHMCEva0EnPvVvAEYnUr1FNvHVtBVNsv.EHfk7pzASxig7Xbk6ZQQxTlaWxXmRRGaGJhQGKPlxeActYvKct8NdgRG+PwLweHGCoEId7932686268t6z5TKL.NwpKCfkQLerC0lBVwvjQ86zvf0F.uDxj5xLHtX+Jls4eCVOzYo1FttX6..DA0..vZTysSzLrz57YdI9rE5K5DuOfknNNXWF.pgh+8WQrDqAB9w7drXGOiffMLb71j3fqymkxi.h9rfdy1zcZX3a3fYXew7qI6M1R76U7w3cwVR.OExLLfQcpDaNRrD..vQVOxbGSXtHjSncRScbjC+6Jh1GD1HrTcpUnsAiPc2zvuElwwLPwp4KY2m3CbU26g8YQdfR0IteogcHt27A.0M9tTsMs9Zzcv9qQbHL0ddaOuLsteslLrWSxtQi+6m9I27u28Gp0ba7NRelZOE9yQQxEq6R9Qe8C+y+c1ozAM7o7omQDT.PgA3tapkTRkfFlVQw0sGGuDhzI1SnnIgYYALmDQCLI1bdgazYBM7wUTd8fP1jzBxUBoP.bIj7oN73BYX+sCv0IVjXlCVpQ32XSLE+Jo5qzgS8M1af5aLcU0WWEYeUemCUWvXYPAnHP9m+IwmeuVVHOatPdA8bf78eQBHGOmpPdbzbUpV8F2XtqCDXdBDOu0cCwtlc5A7m8H5u83K+A5fh6q2OGfOWt.e+Z44qm6MzWOYDtmYCgJCjAKuJBeTYNg.Oa9R02BJxWebE0jIdNJsy4QAbuJldGSlu8owZc8fcIEXOULrysJ8Y9RW6W6DrzUurG8ozUYT.oUUdwojrvEjrP76FxDGqLQYYheOrw1IogIj0kidwffCVgZaI2XZtrATKci7AKPvpVBrai+V1cZIR9zkmDu3bnaJZ60yWasWV9RXvY4qs1KO9B.NnZXifledtpn+FvjHeRq1EyBzJlhaq8N5s.0Hhjhq9+FkD.Sv26p66ZkPtfDzCKfL3KfLdbAj4GVAY.WAYznJHy+tUID3qsDRTX9o7RHuMIfOZRZ2JyMN7J1t0fKc8AdrcsTGaeRzF3.LqOhc0SvmbbWTbb+hNT0XdMkX9wGzRnxi7Yl+wya8qumtbOfQ55tq3L+00dPGm2eurwHb57Pk5R+gE+NL388fMi+qyiZbkkeQMo7roCkxZSbaklVVPO4wp4koYj9uwC4qOYOVs5FWKZvEbXv0owfKz0qrP0pU+DPdAW.ue4CWz5wO6rSv0x5pEIRb9kgB4ytUIt5ktUykCW7LTUhOUuf6PJwM4iReWzEcS74sw7tcR1vKqAgKQ4wClB+azFW68HeU6coAJ+IJI1ypJv3CMhIhtJ8Us3wNDth8K71riGVzTyX45g+h9i6YdBuYK.i8z+4pu7GcddMPZG8EQMitF2Y9bGZHOQvQhyNdfoc1Z45rybSxIb3wGf5P6rO4O+GHWyM0sd7VYr0EanxytyAb4U48OwjeJsi.iMsx57n5b67DQWAO1bzmFUUIRi9+Pdih9N";
}

void instance::createParameters(ParameterDataList& data)
{
	auto& obj = *pimpl;

	// Node Registration ===============================================================
	registerNode(FIND_NODE(obj, 0), "mul");
	registerNode(FIND_NODE(obj, 1, 0), "oscillator");
	registerNode(FIND_NODE(obj, 1, 1, 0, 0), "stereo_tremolo");
	registerNode(FIND_NODE(obj, 1, 1, 0, 1), "sig2mod");
	registerNode(FIND_NODE(obj, 1, 1, 0, 2), "peak");
	registerNode(FIND_NODE(obj, 1, 1, 1, 0), "mono_tremolo");
	registerNode(FIND_NODE(obj, 1, 1, 1, 1), "sig2mod1");
	registerNode(FIND_NODE(obj, 1, 1, 1, 2), "peak1");
	registerNode(FIND_NODE(obj, 2), "mono_gain");
	registerNode(FIND_NODE(obj, 3, 0), "left_gain");
	registerNode(FIND_NODE(obj, 3, 1), "right_gain");

	// Parameter Initalisation =========================================================
	setParameterDefault("mul.Value", 4.04);
	setParameterDefault("oscillator.Mode", 0.0);
	setParameterDefault("oscillator.Frequency", 7.26824);
	setParameterDefault("oscillator.Freq Ratio", 1.0);
	setParameterDefault("stereo_tremolo.Value", 0.0);
	setParameterDefault("sig2mod.Value", 0.0);
	setParameterDefault("mono_tremolo.Value", 0.0);
	setParameterDefault("sig2mod1.Value", 0.0);
	setParameterDefault("mono_gain.Gain", -6.10691);
	setParameterDefault("mono_gain.Smoothing", 20.0);
	setParameterDefault("left_gain.Gain", -0.0574055);
	setParameterDefault("left_gain.Smoothing", 40.0);
	setParameterDefault("right_gain.Gain", -36.9161);
	setParameterDefault("right_gain.Smoothing", 29.0);

	// Setting node properties =========================================================
	setNodeProperty("oscillator.UseMidi", false, false);
	setNodeProperty("mono_gain.ResetValue", 0, false);
	setNodeProperty("mono_gain.UseResetValue", 0, false);
	setNodeProperty("left_gain.ResetValue", 0, false);
	setNodeProperty("left_gain.UseResetValue", 0, false);
	setNodeProperty("right_gain.ResetValue", 0, false);
	setNodeProperty("right_gain.UseResetValue", 0, false);

	// Internal Modulation =============================================================
	{
		auto mod_target1 = getParameter("left_gain.Gain", { -100.0, 0.0, 0.1, 11.0 });
		auto mod_target2 = getParameter("right_gain.Gain", { -100.0, 0.0, 0.1, 11.0 });
		auto f = [mod_target1, mod_target2](double newValue)
		{
			mod_target1(newValue);
			mod_target2(1.0 - newValue);
		};


		setInternalModulationParameter(FIND_NODE(obj, 1, 1, 0, 2), f);
	}
	{
		auto mod_target1 = getParameter("mono_gain.Gain", { -100.0, 0.0, 0.1, 11.0 });
		auto f = [mod_target1](double newValue)
		{
			mod_target1(newValue);
		};


		setInternalModulationParameter(FIND_NODE(obj, 1, 1, 1, 2), f);
	}

	// Parameter Callbacks =============================================================
	{
		ParameterData p("Frequency", { 0.0, 1.0, 0.01, 1.0 });
		p.setDefaultValue(0.89);

		auto param_target1 = getParameter("oscillator.Frequency", { 0.1, 12.0, 0.1, 0.229905 });

		p.setCallback([param_target1](double newValue)
		{
			param_target1(newValue);
		});
		data.add(std::move(p));
	}
	{
		ParameterData p("Stereo Amount", { 0.0, 1.0, 0.01, 1.0 });

		auto param_target1 = getParameter("stereo_tremolo.Value", { 0.0, 1.0, 0.01, 1.0 });
		auto param_target2 = getCombinedParameter("mono_tremolo.Value", { 0.0, 1.0, 0.01, 1.0 }, "Multiply");

		p.setCallback([param_target1, param_target2](double newValue)
		{
			param_target1(newValue);
			param_target2->Multiply(1.0 - newValue);
		});
		data.add(std::move(p));
	}
	{
		ParameterData p("Mono Amount", { 0.0, 1.0, 0.01, 1.0 });

		auto param_target1 = getCombinedParameter("mono_tremolo.Value", { 0.0, 1.0, 0.01, 1.0 }, "SetValue");

		p.setCallback([param_target1](double newValue)
		{
			param_target1->SetValue(newValue);
		});
		data.add(std::move(p));
	}
}

REGISTER_MONO;

}

namespace transient_designer_impl
{
// Template Alias Definition =======================================================
using convert_to_mono_ = container::multi<0, fix<1, math::mul>, fix<1, math::clear>>;
using fast_envelope_ = container::chain<0, dynamics::envelope_follower>;
using slow_envelope_ = container::chain<0, dynamics::envelope_follower, math::mul>;
using split_ = container::split<0, fast_envelope_, slow_envelope_>;
using analysis_ = container::chain<0, filters::one_pole, core::gain, math::pow, routing::ms_decode, convert_to_mono_, routing::ms_encode, math::clip, split_, math::mul, math::add, math::clip, wrap::mod<core::peak>, math::clear>;
using signal_ = container::chain<0, skip<core::gain>, core::gain>;
using spolitter_ = container::split<0, analysis_, signal_>;
using transient_designer_ = container::frame2_block<0, spolitter_>;

DSP_METHODS_PIMPL_IMPL(transient_designer_);

juce::String instance::getSnippetText() const
{
	return "1477.3oc6Z07aaTDEeVmLMItABPJRQHP7Q4POfrhiAUtfrKtJUQJIjVmRUNYM0dR7pr6NK6NtAW3.RPk3FWAjnHj3.WfiH3Txe.DIjhDvQ3HhKAgDGPpp7lc8ZueLNdS55ORj8EaO67wu4898dy68lcUVUJRYxktJR4BXtEwvVkZvKWkZqtkA0BsHoBmY0XMBuFRYNbElAmnBOHylVDc5BkusFqx1nUqqWrFwvfpYiTvJoPHzxP6AZVYrUg0pHSiU2RzInOnhLccX4PJovd+9VpUEKEV4UfdTpFam0HhUhSsDyQJ2YwFA+d01P+7XaSllJmGFwOoODaaB8nmA02rgIw1lVUL51fLkOPlFSLHZMrUs6LFqTC9tGiwaPzMWWUmtBLKSMNx4yqVnMleLWLm1EypFl04kqYFDyOEdSUMgNICyfVFj8zABn8KzWjoU04W.GYEV05ZDtJyXch0VTNrsP93QvT25evzsjwcnVb2YYEUi2lnUm5eUVg7tAa6iOnvxrcnVKqpCLJe87lllgaE5aIN0rj5ccF+m+YhO+X9RaS2wUhJZc+2+Ou7GpuedWt7hVz2oN0nRCTqk0YXMtUATbvcNmOGj2Ot2+mDetXHb60yf31qucG2+28+l+8k96e1A2iguNJjn6v7n3KmWXuHxYzBRkyKrmD4Lz2tiWOXIv64vWSXqEZE8ST5JjQnnP9v7xfLBICxGl2OjeuK7cW6Ot6GEBxOrwWt1rW8flTiR5LFulpwVsws2vFX3tUaGgndEgKEIrCKFL6bUgeGjqamoDCXlVtJKukPIEv2y4A+kVzLaMn7T1xQyXcvQiuMkRpl+qAXfH1YSiuA0lxcEEQTKqU+1ZpUDSMJz3lAeSaZbGpeOcohOs3RRr.ujTKvMjZABmW2EKvwvYQGgsWgGMNbgOoPr4vPe6t6htZ64QI7ycwXS1NAYrow5v2YDsOPIrw4jQkgK2e9ciLAVJ2WtejztJMcaHV1JhVBnQlEawpyAcYl1c3zT.LczyoSvjBkVYNqrNyf04XM0qqwU6KwC6emEJx3llL.VjZxHZODBU525EkTmMMZ7o.l.WQiRBk6zztp.2mLRIzCTBwvnNsaPCfWJHg.nkrGgeL2db1vOlfQpZlMbjeMYjpliNHsecP5Dtivs5ICnhqHufJyf2jXyAZ+cnZ.rGtqphRfzab.9lLMMgNNHveVb0FFDc0J1Y71Ys64PY9NmvzM5GQv+ZeZwWb+uvsvJShuBmSprcRmxeOEzSA4KBG.a6yX8a+ku+2dNkpEBZr1MupMsXrAdzoDKlTArXb.9HKlArEyreE4eV5AOcgyBVLAJMup6dvJtYgzmrERxnLNb2ginL1MtNthYfhVB4yPkdqWEm3.WC9Asp0bGzFXLoZUo5BQ6ihYOwsl98NqKNmSJTixfZfmAUSsgIkrszaxv4AC5r1kpOTTB2HLbAFVBFyi6E1k6czzRYIdVyB7O7biew6vjmn88NcLKH1HqmjobXG6TZlDKdSUHZmJxkIsWI50MoF1DNcn95M84dH0Yrq5bNItdlq+bUmO+89qq+C2e2yBW0YnS.FcK8IB0McRcpYO5V5QQiI.ToCUzY77YxMe14yg5Hc9qMeiz+5EelBmnzOCn5F22NG6RBtRyfHdAG4WbeQKZuqv.9yF8siPQoHCrVpHj7Nmv29uvtzKrrvu6LxhKqHyqbGP5YKUErqTAN7aYtdCSwoT3RdD3j5EFQdbCmr.3ZJlaUbp3EOyF9VKAAu4iGGmadTbErAqAY.AaKzjPh1duQRZfjsPtr4t7qihHaaWCsnHaiBxPlTgq2q.48hPcS0AIbv6EIfDtMlNUJhO9TpS4a3vbpowq685kaihassjkWQjpQElbMlTYsWgBCHi8xDIgjvwu.cGt6iZhTxYTQStHo7+mJ51cxntBPStmrs6bRCEYtii+eoa2ipFD9O2Ogzux1vubhZAI7dFLXi+G7Aq9vA";
}

void instance::createParameters(ParameterDataList& data)
{
	auto& obj = *pimpl;

	

	// Node Registration ===============================================================
	registerNode(FIND_NODE(obj, 0, 0, 0), "input_hp");
	registerNode(FIND_NODE(obj, 0, 0, 1), "analysis_gain");
	registerNode(FIND_NODE(obj, 0, 0, 2), "pow");
	registerNode(FIND_NODE(obj, 0, 0, 3), "ms_decode");
	registerNode(FIND_NODE(obj, 0, 0, 4, 0), "mul");
	registerNode(FIND_NODE(obj, 0, 0, 4, 1), "clear");
	registerNode(FIND_NODE(obj, 0, 0, 5), "ms_encode1");
	registerNode(FIND_NODE(obj, 0, 0, 6), "clip1");
	registerNode(FIND_NODE(obj, 0, 0, 7, 0, 0), "fast_follower");
	registerNode(FIND_NODE(obj, 0, 0, 7, 1, 0), "slow_follower");
	registerNode(FIND_NODE(obj, 0, 0, 7, 1, 1), "inverter");
	registerNode(FIND_NODE(obj, 0, 0, 8), "ratio");
	registerNode(FIND_NODE(obj, 0, 0, 9), "add");
	registerNode(FIND_NODE(obj, 0, 0, 10), "clip");
	registerNode(FIND_NODE(obj, 0, 0, 11), "peak");
	registerNode(FIND_NODE(obj, 0, 0, 12), "analysis_clear");
	registerNode(FIND_NODE(obj, 0, 1, 0), "compensate_gain");
	registerNode(FIND_NODE(obj, 0, 1, 1), "dynamic_gain");

	// Parameter Initalisation =========================================================
	setParameterDefault("input_hp.Frequency", 93.9);
	setParameterDefault("input_hp.Q", 1.0);
	setParameterDefault("input_hp.Gain", 0.0);
	setParameterDefault("input_hp.Smoothing", 0.01);
	setParameterDefault("input_hp.Mode", 1.0);
	setParameterDefault("analysis_gain.Gain", 2.0);
	setParameterDefault("analysis_gain.Smoothing", 20.0);
	setParameterDefault("pow.Value", 1.0);
	setParameterDefault("mul.Value", 1.0);
	setParameterDefault("clear.Value", 0.0);
	setParameterDefault("clip1.Value", 1.0);
	setParameterDefault("fast_follower.Attack", 0.0);
	setParameterDefault("fast_follower.Release", 160.035);
	setParameterDefault("slow_follower.Attack", 5.49735);
	setParameterDefault("slow_follower.Release", 160.035);
	setParameterDefault("inverter.Value", -1.0);
	setParameterDefault("ratio.Value", 0.01);
	setParameterDefault("add.Value", 0.5);
	setParameterDefault("clip.Value", 1.0);
	setParameterDefault("analysis_clear.Value", 0.0);
	setParameterDefault("compensate_gain.Gain", -0.03);
	setParameterDefault("compensate_gain.Smoothing", 20.0);
	setParameterDefault("dynamic_gain.Gain", 0.0138037);
	setParameterDefault("dynamic_gain.Smoothing", 6.535);

	// Setting node properties =========================================================
	setNodeProperty("analysis_gain.ResetValue", 0, false);
	setNodeProperty("analysis_gain.UseResetValue", 0, false);
	setNodeProperty("compensate_gain.ResetValue", 0, false);
	setNodeProperty("compensate_gain.UseResetValue", 0, false);
	setNodeProperty("dynamic_gain.ResetValue", 0, false);
	setNodeProperty("dynamic_gain.UseResetValue", 0, false);

	// Internal Modulation =============================================================
	{
		auto mod_target1 = getParameter("dynamic_gain.Gain", { -18.0, 18.0, 0.1, 1.0 });
		auto f = [mod_target1](double newValue)
		{
			mod_target1(newValue);
		};


		setInternalModulationParameter(FIND_NODE(obj, 0, 0, 11), f);
	}

	// Parameter Callbacks =============================================================
	{
		ParameterData p("Analysis Gain", { -12.0, 12.0, 0.1, 1.0 });
		p.setDefaultValue(2.0);

		auto param_target1 = getParameter("analysis_gain.Gain", { -12.0, 12.0, 0.1, 1.0 });

		p.setCallback([param_target1, outer = p.range](double newValue)
		{
			auto normalised = outer.convertTo0to1(newValue);
			param_target1(normalised);
		});
		data.add(std::move(p));
	}
	{
		ParameterData p("Attack", { 0.0, 100.0, 1.0, 1.0 });
		p.setDefaultValue(30.0);

		auto param_target1 = getParameter("slow_follower.Attack", { 0.0, 1000.0, 0.1, 0.231378 });

		p.setCallback([param_target1, outer = p.range](double newValue)
		{
			auto normalised = outer.convertTo0to1(newValue);
			param_target1(normalised);
		});
		data.add(std::move(p));
	}
	{
		ParameterData p("Release", { 100.0, 1000.0, 1.0, 1.0 });
		p.setDefaultValue(689.0);

		auto param_target1 = getParameter("fast_follower.Release", { 0.0, 1000.0, 0.1, 0.231378 });
		auto param_target2 = getParameter("slow_follower.Release", { 0.0, 1000.0, 0.1, 0.231378 });

		p.setCallback([param_target1, param_target2, outer = p.range](double newValue)
		{
			auto normalised = outer.convertTo0to1(newValue);
			param_target1(normalised);
			param_target2(normalised);
		});
		data.add(std::move(p));
	}
	{
		ParameterData p("Transients", { -1.0, 1.0, 0.01, 1.0 });
		p.setDefaultValue(0.01);

		auto param_target1 = getParameter("ratio.Value", { -1.0, 1.0, 0.01, 1.0 });
		auto param_target2 = getParameter("compensate_gain.Gain", { -3.0, 3.0, 0.1, 1.0 });
		auto param_target3 = getParameter("dynamic_gain.Smoothing", { 3.0, 10.0, 0.1, 1.0 });

		p.setCallback([param_target1, param_target2, param_target3, outer = p.range](double newValue)
		{
			auto normalised = outer.convertTo0to1(newValue);
			param_target1(normalised);
			param_target2(1.0 - normalised);
			param_target3(normalised);
		});
		data.add(std::move(p));
	}
}

REGISTER_MONO;

}

DEFINE_FACTORY_FOR_NAMESPACE

}
#endif

}
