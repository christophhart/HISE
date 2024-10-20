#include "broadcaster_resultpage.cpp"

namespace hise {
namespace multipage {
namespace library {

EncodedBroadcasterWizard::EncodedBroadcasterWizard(BackendRootWindow* bpe_):
	EncodedDialogBase(bpe_),
	bpe(bpe_)
{
	loadFrom("5771.sNB..D...............35H...oi...eYA........J09R+fQbosJK.5SJ3a.Cnrx31Pr5iaJBHIMbmVa4RVjTtMsMhsH38uWCLaedCBypjOnKY6Hv0gwKGO83EgWfzADZ.pFv9J9kw2VmxSMoB79JAsMwADDQDaPBX0sXajkFHgChPDR7fG.pY4dABzdstg9F8nB6GeN2fudB6KbW0P8lYQxsOrqaaz0Ab5+jSTW+nNUxLWpGJMRUw8.uPtTRB+sqz8fSWt1QoTtl0pHhHBEfUV7qcPdnRmGTFDT+TTtYrxTxZ9tl.C.IjPytPnJszs4gOT1iqekxT+At++4PI5W6lG3UhoNIaBrxCPn0OVggVV9.KUZXfdg0UU17laIrxDakxmGdtefP..AD+qwK9.TBrBDuqU1dNCbGP3iFDQr.rxGAsZz2A2AF3NbI8uvsRAbGSxOW4qZ7JjKeELAhGt7hW0HL+qmRInTmkOsVErzoYI.jIWxqEbutScfINaRsEUJmvqzI.Jkt.g8667vrb4E1G.c56oZksyBHXEHttuSasaCHH1vjLfXC5y8rbUZCi5MvBhvAQPQCbfxKakZjxqDfHhInXBHb5J0HnLm7c.ELvARPwDTDACn3+PeGz.Jb.CnvCdYS7fIXPCdfE5pURDwDSvfIVvChfAXEHtWyZKPNPP4AK.q.QuvhYBjC3YcJlGbNwT.KVaADDQrgIP5Ok.DrBjm6LffHhMfArBj7yKT1ziABhMfg.PpConghjLquZbTgWZf1yJ2PjrTMszyg+9gtvTR53FkwD4kgrFswUHNiTH9PkUQ0QxlQuqWEafNcsLL3DmiHnSi4mKth7yKr7XoCHwcJawYDbNcPZD7alitecMN83ms0M.pbOAKfvRONw1C8HijHP.D.AMGYTto.Ast2UkyZ6ybIptHU9epAUOU1FMTAm5cGYwa3GJqAwr2iuMFo1nMr8wekgT7DhW263SiuuLboe5xl5XNkNWEp6xMdGJ4vWNgbMpqVgB.F8QsThWj2zQN5aEXpXURpsrXcFHKWqJb59rOqqEi5aDv0xx2eRnJkLRw0LHdcLwm10OOKr7CeUxXpxbxsebo9CUGswzHrT29rfRAJTn3vXL8MILV6EWdM9BEEl4B2YPW+7hMV+L5Q2U0SiKV+k2GcgXh5uL2JbwIN0vImpQwoGG4TguNBPJNXhP79o5T3SidSeG0nGifdsK5UOxGYrMb7enbva5kGtTCHRlHQD8SnUSxRh7D4YgfD18d0IIxCNARGJSw.oiHLVZYwqCkmUompWnAfOnK0rZKqKWboCRs6g8.cmtR203D244ejOO4OfTjv3Nh5xEorQpTwm9UMN8Yg9SmxJDqIopSoRIuV7A0OeLoSWJx85rJsKzG.zWNn+6hSU9ruJy9ejfShfSsIF6gJnLQ+jgIN4h5+L3jLKVyfpGKyUIsNBRQcZDlYAYPdQ2yFOoiZpyidyit1ZlpgUoFCJYod5fsSuPDD+lgAxji9rhu+l.bwIpha7SzyfkKZTwDhfKvfS8M4FIJstvcPQBuOqTctmR0Wtfhj1s3fCOfP3N3NbI8U0Rg6G2zL0Ikb8CuOHEk61Cc+tzrBrBcq8TMYjOsoiPkg3AJ2LEGxkMbhxW9W2S.UfBTPjhSmEPbOYVfLgDSz.JP7.DJRbOHES4HnD59XcepwqUKTyvHkiJT1dVkVsiKtfPnCWn1qcIPRAzoOOOG7SGlpcC0eN3vjh0mETWFY3ZzAGPGJ3fC3jJd0yDA0+47IJ4zMT1a0Zv.XfVXcuJ8pqNbh9JSWHAfKNcbIfgEvvC0xjvD805wpF8iLC4obCScn5ZILQe.9EaGcW1d74WqTo2NKOfoeY6R+mR+PYsSi9jnma.PFZnBRrwydtRgEqi1fhDbkn5QJ0VltlZ8Ri7k2LP2U2FRPmBoeET7TsrVKW86KGntVGhaVxcvcvcnLmWuRTkRe8zBLCuqm8xG8hlC0Mw7LJBtbY6Lod7TIi+TcFp2LJRxfYVWxf4pAWpZwmMhaw0RsKC0WRzea3yhKNi3UxvJt3MrnRsiRVtbSj0Dqeze0m7E2J0WWJBtTo8Q2D2pj03UUMoJALcWcsOzINWfLYRr0eEfP48jo3I+Q+U2+E6GkjQRjm2rotLqNxg+qQeJxU2iRg+S1S4OI8BqT+9ySmok5vkjkNkL8hHnOwzO2kJIsXEs0CLMze8pQcMW0rp5mvibHggV2ykqQvLYd0SbsIx5Z8eUnrwVSoJQ0r9io9Pe5LRhShfMeCFChWHnzcwjHfBvDiW.zwdBoxqzhoZTljLVhva9NUg+Sp1F8VlLYZUpNbwdJFL3HiL3Nn.NohEmVDAcKnTX4yeKcezyVgeynq8RcZ5n3hUu+MP00HCfwygI5Te2LQ8WKoO9bwB9OpDPtmPiOZm1w4gAmvTN2HBhAlKQfsOGKAFtBpSkclZXNmRMz.....BLEX.fAKTwkIc5foKj5AyCj7gxHEOPLZDRNJ.iw.HJ..v..C.P.FfP..zPSsCLTQhf24CND+TJbJBj3WvMmxVs9kppwcbgdrLFLtQi.j2s0SpleZOtmXgG84U+vgw4AcqiFB1AbC2p6vMH4j4hD+5uRE1L6bzNH5wkZIRJjTn.LkBnJwA0ALMM6hx.+Hv3Kf3L3ptKkLphahip9f2k5RsTlMuJK0a8h7eY9tDKT9dB81Rwko5RpR4qmpxqRsf5pVK.L2HMkHtRdmttjZGhZTfeEPPIYkHEODpTMXOOBGJPPGGgGL7G.nZlBFTJ.kQxBZvOnnDfVb7+IWHjqL.LDUOT+ALEMAm67rggEw0LZ9KxH7Jz7jPTqfXxwhHm.PmI0H0Vm2Glts1SegNHrgL9u70p+7my1Hetk4msZHuIT4Ln4Pkz1AeDQrBXvA.eiGpp0Du9wuSn4gM.k9w2n5GoV.MS8P1L7P4B2IGq+Kqh50GAlCwyenfb6OSeTyCDFCfoX6SgSn7xf9O2gCKUNV88uNcA5G.tyKyXtS04KhCadddSb0Q3XGpsTJIdO7UqFwyXmv1h8u3Ni35p4hofKoydSSvDlWyet5HLUtz+Lg+j81Me9wCX1hLSVjzPoriECtBP9Z1rNjhRkbrzJjV1qiTExnd1Y8D09Sq6j5W2uZxgWg8endTvuD860Jhto0yIrkbaW.MEoECY1oPdesKva+.rkCaukTJ3o717JTnO.Gs0EHczZ0P5HAic1CKHk9cSL9zVGdAgcA6X05B01ZezAch1QPHnR59TSuZbRmVi4Bf84RsCNX4Q3ZObtxDQGewuM.2YY8hfrBxATeDgZ7lDOMPyfqSN7EkG9kkrZ9rMTDa4Bu0qJJa4BvqvVCf9EK5Bt11NDHJ7ZaK7xqNrhJOC865JOatPtp.sitkWQ87Cqg8vjgKYpmkZ10WYN32KTuVsBCoVoXafw2y.c6dcAQL5xmszgNRN2b4GKSpAvJI+ibMicMWNC6CsXMJ0z10MYy8xy.HOfK6UhedJdvo8ubhyD4+apCnD3bWuX1zTUQ7oqX+DLUK.ysPCugEuhnq8EIuP4vhDQu0pIMRL10KLgg8bJxjM1ASCi8hKsRBnCcU2XjgmCJ3Aj7J5ULdl2PzVoXVwDPVdoh63d+qjVMdGsP4+63fCNsJOMRzUfssJhd9OGSj33n3I00BQ2nSDM0opaQZPivvnDTEcjKG3hjUxq21fTgPfTA7Pb63h2.KOVkf7ty4S9Zm9DbXBKh9DH8Yj7FQALlHaW1xL0ZEnzEUsQNfw7IgXz.0ULOViVqzfXkrpBoas8PYN2Me6zFbyTdyskWs5DwqtBGVbz5QVmPOl1ACm6Xh2AupXfg9U8jlKAiAZp9nMZYlFWhxHN.0cukiLcrtiK9a748cJvBbZI0AKIoIvxq.M2lQaWmHqaqldM0TZO0e+0.AcEJEMXBkqRAqqPoBP7Dbir36qVZRIplSP7SunwEHK.Ql8iwTLmFKYEwZECFsmDl2b4DH2qjrzBRIBK9vReQnijO9FqyZpWQSvwjWSKXw92HFZN4tFoFZu+e7xl8BDA2flO8yieIADxJmq1igELMcSGpygVSF70WVI3v7uR9e.t9Rmp+rVS+73xMq3HaxKLTDFnZcTFVCkg8H.CoF1H0rIwvciRr9.suQYylDiPlods2OH.GivWDvjUzwERXeMAPafBnOBKrh9xPUbrpFceZ94DELg8S1EqkZF7IE6O7OoMof7wECbYTmGOJSW0BYYQXoqXDxZFoqBw447WTq9opA1huGVtvfjdZ+fSMjvgJ0mbHRAFyv8AiINrk54Enwxtyifc+e6QtNx9jHqNnvP.B.3y.CD61W8+QTWug9sH4fTYQYIWQTtVzZkbzp1WwyyMIvXoAvhZd7URS.1SfDKriWG9+f7I4dcTlK.P.10K6rtn4HnHwWVQj0RAAFPh0KO1owaMH8ACblnmHmfAEQJdgOczdSPnKFqU2OJWL0BbyaFsU5CwCGgv0RSriA8fwSR35hZC2eaP9hzpBC4eLg5wjHe3n97egB465EOCznw.QYVUpcFw7uJzH7+kN1dB1OSCFgHLE.gksbtcvJxThpH.mOea.icgl25yRsRB86ieDExMVDOm2.yQdk7+RlTLkPikcXOZjX9NiZjx3qVU8rcjK4WHXeprgKUwb3J.rjDkFKpi6GfgmKXRAoFa8bj+oYlx3BM5xBOA+oUuct5vfCiTM.beZTW6XabdU8eCyJl2YTGFJY5XFfz6.TaPjKWUD053tjrytpyC0TDM.LMb+6PHeVfh.Umws4zANiIwk9hZbm+2VfQM7W60qAshGRBL8mRVXXd0H90SMafntj4jkpcYbIj+LeEu0U4uamX9FVvaUhnm5Az+Vkz8hyzszGiHLzQlHuEHtVGJ0iowllj1pHZs3+w.Pk8w5bMHFLB3PRCb32hf6r0eiqSNOq3UePMc2e8pqpTCl4wGw55vkcqDq0dpsZE9OPl9ySSNFoKUZ.0FV2YBI162cnW3geaAoAKHJNRfEPWN5z6speDitIEsqltbt4Ihq8eyKObmEKQt6m6BR+mmyhEKoFHsnoZghmA5fDtqPieR7TkAJs0GSODxzV4XoL99SlPyLCH+CTvvQ1KdcICCidL82TqII80O9SLnxZyJHGxj2r1njh9NLf.Y75e4uHuKqavyeOemMPhKJZnqbzMYqE36Y392PtG9PRBPkD2+kIPBiG9kTQnna0RHDl6pszM+fwxEgxHh8oR7+S0+rEnzqsCIvEEYTwQaInxPD0MfSnmpRNBBkPUa9499mZiUqsQwiGSR++SD.tZn5Tp6FVoquNImxPmcC344U9WpHus2MisUGJcNvzy45DipvpQu2DBn9+SbOkInRVV4o6dBvJyV0k3VNjT5An7r9KqtDvGgnVEcDegiXiBT1uXOwrnmVXZRKfglvz.fSdQUuEHJUepDfUnC2pnpE.YtLeRpDuIioNVOLOvK3+OivV3hDEyDkEmKWvYvaB9Dj9u39kGgK3IUZLWuI9rfyZ.xEHYWfNLwEng28pyce791roF2nsu9ko93TE3ePqF6EM7KxF69r5dPk1fP1GiDIOS6Lxraujs7r6CfJZEBOVdDqQmH4Jmc81vMoADDckOCAOfACOZRRBX5n.iKzg7vIoiR31+kToFbszY66hqEvqsbIoQp4VvR1LNCpmgYeC32W6nLHtxqBNR2dDgVpiYKCoRdTHXYhLRa6E5bPz4WWAXJfeaMwNO7jWr2619BKzI.LPpJhk9hQVyScpPmRi5IUOdhBs.BpoVV5xj3wJIEWh8Z0OQXaALDXVwBeuovDMqU97WuA+x82aj0Sc6sVLyYSkbqELvLMUOQOP63EzZ+WE8rB78eQ7ntXENEKkRv8H.MpjRBktIJc3Imv7dRQP55WdJr+cbEjzXQzJ5EqxonL8kyvVlBdEvSU9xCNSx78346JKVY8NvKtvZjzH1AnL1JTUSNVvrL73GUyrZHrYvH1zPEdU9.vXFVwD9XRx+gyimQfGvrRnIty5k.KsveG7QMVrNON6b1Y3iPD.rv8eJD2dvQHw2tc9CCsLtgtFLUB3C8gerDFAl0rZ3c1v2AKVlOx7EyawZvRNYNFaoCxQI0+FDChRg3N4ALWH.zhSo7Uy8u1HhOR34dLsP5Pgx3VseoMHiuw2+QbnySXHs2u5qlAiJEetNyFvdtW4oDeo.dNIBnuA.aiaz5pPk6GxqEfZ.eTbaSdDFFdoyW.3hfOi06KMJnHCWrXX7Lzmjx18dcPAmjdN.oX2ImsSpreUyumEVdP2.oHObEkyoWXBrJHUP1EvF7M.vnJiLTkSb6b+ksFnfdnwsFsW7bz13emTmw9NLdd2ybHaZLMaM4DlB8ECGqEPLM9svYq8eGaanOEvFTYo5YGLIghVLulfsis+XKsPlB.sYibnz8Fg0x20oxfu8ubTslvfJukV2YABanFnIJTrdK2+omdYSzs2CCynHKdPDJ43PRDshx6GrU5vcoKyCUUJulabL+GoRzp8.2kR1MtcdmwIxp.rTBgu2X7SGkvtEf.GzoATJF2AkMWo9x1a97uKwMf03Gy5.EbBVxffM2aNSvQf3sbThwvUxyPlDvGlVZ3Qkn5XzstmO7MjrfXnsAKTeIlqT7oA3k7kKEdhVi9N+DE3fikP5VJx.L53jXJHsIksZfRZKGFvIXXsdPIBj1QZsQD5xwfR7gfHvMoQv4f4fed+Dt5JOMjXrwgrAd91Lf+ayY25u6xVyFYbj4rhnD5JF3xr1H9dLOkWa5.1gHh7nVh3ksylYuOx4pG6r1UPkwOAzam4SHuK7YFQOQiZNNzTWkLcQ182MEIxPBBYwIvjaS7hCXh.5Csdug2wWHWRPVRtdazhqlWYQHO+.SyxsePoda1X4eJ0ui0Da65q3NTCloQfEe2HoZwS3p7+IVMi1KVTHuvWauszUS464KmVtpcs2tcwhGmld9E6KbQKzLUTm..ZQ3iuwEg8oYC8TfdOeVVJ2HchOLzLA8oTLTiIKLwQcNgQ+1fTYBRMdbKym5MIyPL5YpBSzNI1iGwTjMEDRQQUvQCXI2QLBD23KRzcZfzlgnmJLou9mZtjZ1xzezNtwVHfNFojNtH2x7AoRohK4HkAw8N5JJmwzmWbxJuvNCxe6F9hB8RuUlxTFjJpjng5AdeoBO9M19ZbUoXjHfCFsvE1zzDrze4dgt138QIt4v7ktdVYsvMnLh27JkfpGUDL7lAX8fJZNwZVQ24rHIY7Atq.NftDO0thblWGebl1aYbTIaFFUg5U1f4APNxjDcg.OEYkFDWWeD9An0XW27OW6PW0UIgfOXqRJXzcR7HPJXpKqCpaw.twNW467Qz3b9grAsgUATx.jy+hoIlHaVfawjQP+d+ehfMcfvyZ7r2kLn.cPIvDmae+gjafVUHPQWLMuW20D9lZEji84KyF48A8SOYLsFl5MoXhvEgv8fmcSYhVaNdqkcJ0UYUmxiqYsH.xLdbEzTZXxF9nibUaFlk.QK+TUofFl1XyU87.92zf2fRdw2xSiId3MIoFihCxqDj92u5klseTwdvKyo8PhLv.Yso2d4D6lVr7echCgYhKQULj5rYrmt9DFetum+6xiPANLhT1eMDbCzdUO9Re0+tC3+KwWPrZDEKzgXQIvzCaApI0idFn4DnAcLgcoCiry+M3aaqEB.b754XFKehym2S2qv5G1cHk9Poi...lNB..v5H...");
}

var EncodedBroadcasterWizard::checkSelection(const var::NativeFunctionArgs& args)
{
	if(auto fe = dynamic_cast<mcl::FullEditor*>(bpe->getBackendProcessor()->getLastActiveEditor()))
	{
		auto s = fe->editor.getTextDocument().getSelection(0);
        auto selection = fe->editor.getTextDocument().getSelectionContent(s);

        if(selection.contains("Engine.createBroadcaster"))
        {
            addListenersOnly = true;

            auto obj = selection.fromFirstOccurrenceOf("{", true, true).upToLastOccurrenceOf("}", true, true);

            auto cid = selection.upToFirstOccurrenceOf("=", false, false).trim();

            cid = cid.replace("const", "");
            cid = cid.replace("reg", "");
            cid = cid.replace("var", "");
            cid = cid.replace("global", "");
            cid = cid.trim();

            auto p = JSON::parse(obj);

            if(auto go = state->globalState.getDynamicObject())
            {
                go->setProperty("id", p["id"]);

                String args;

                if(p["args"].isArray())
                {
	                for(auto& a: *p["args"].getArray())
                        args << a.toString() << ", ";
                }

                if(cid != p["id"].toString())
                {
	                customId = cid;
                }

                go->setProperty("noneArgs", args.upToLastOccurrenceOf(", ", false, false));
            }

	        return var(true);
        }

        
	}
    
	//auto content = BackendCommandTarget::Actions::exportFileAsSnippet(bpe, false);
        
	return var(false);
}

void EncodedBroadcasterWizard::bindCallbacks()
{
	dialog->registerPlaceholder("CustomResultPage", [](multipage::Dialog& r, const var& obj)
	{
		return new multipage::library::CustomResultPage(r, obj);
	});

	MULTIPAGE_BIND_CPP(EncodedBroadcasterWizard, checkSelection);
}

StringArray EncodedBroadcasterWizard::getAutocompleteItems(const Identifier& textEditorId)
{
	using SourceIndex = CustomResultPage::SourceIndex;
	    
	auto chain = findParentComponentOfClass<BackendRootWindow>()->getBackendProcessor()->getMainSynthChain();
	auto sp = ProcessorHelpers::getFirstProcessorWithType<ProcessorWithScriptingContent>(chain);
	    
	if(textEditorId == Identifier("moduleIds"))
	{
		auto attachType = (SourceIndex)(int)readState("attachType");
	        
		switch(attachType)
		{
		case SourceIndex::ComplexData:
			return ProcessorHelpers::getAllIdsForType<ProcessorWithExternalData>(chain);
		case SourceIndex::EqEvents:
			return ProcessorHelpers::getAllIdsForType<CurveEq>(chain);
		case SourceIndex::ModuleParameters:
			{
				auto sa = ProcessorHelpers::getAllIdsForType<Processor>(chain);
				sa.removeDuplicates(false);
				sa.sort(true);
				return sa;
			}
		case SourceIndex::RoutingMatrix:
			return ProcessorHelpers::getAllIdsForType<RoutableProcessor>(chain);
		}
	}
	if(textEditorId == Identifier("moduleParameterIndexes"))
	{
		auto firstId = readState("moduleIds")[0].toString().trim();
	        
		if(auto p = ProcessorHelpers::getFirstProcessorWithName(chain, firstId))
		{
			StringArray sa;
			int numParameters = p->getNumParameters();
			for(int i = 0; i < numParameters; i++)
				sa.add(p->getIdentifierForParameterIndex(i).toString());

			return sa;
		}
	}
	if(textEditorId == Identifier("componentIds") ||
		textEditorId == Identifier("targetComponentIds"))
	{
		StringArray sa;
	        
		int numComponents = sp->getScriptingContent()->getNumComponents();

		for(int i = 0; i < numComponents; i++)
		{
			sa.add(sp->getScriptingContent()->getComponent(i)->getId());
		}

		return sa;
	}
	if(textEditorId == Identifier("targetModuleId"))
	{
		auto sa = ProcessorHelpers::getAllIdsForType<Processor>(chain);
		sa.removeDuplicates(false);
		sa.sort(true);
		return sa;
	}
	if(textEditorId == Identifier("targetModuleParameter"))
	{
		auto firstId = readState("targetModuleId")[0].toString().trim();
	        
		if(auto p = ProcessorHelpers::getFirstProcessorWithName(chain, firstId))
		{
			StringArray sa;
			int numParameters = p->getNumParameters();
			for(int i = 0; i < numParameters; i++)
				sa.add(p->getIdentifierForParameterIndex(i).toString());

			return sa;
		}
	}
	if(textEditorId == Identifier("propertyType") ||
		textEditorId == Identifier("targetPropertyType"))
	{
		auto pToUse = textEditorId == Identifier("propertyType") ? "componentIds" : "targetComponentIds";

		auto n = readState(pToUse);

		StringArray sa;

		if(n.isArray())
		{
			auto name = n[0].toString().trim();

			if(auto sc = sp->getScriptingContent()->getComponentWithName(name.trim()))
			{
				auto numIds = sc->getNumIds();

				for(int i = 0; i < numIds; i++)
					sa.add(sc->getIdFor(i).toString());
			}
		}

		return sa;
	}
	    
	return {};
}

}
}
}