namespace hise {
namespace multipage {
namespace library {
using namespace juce;
Dialog* SnippetBrowser::createDialog(State& state)
{
	DynamicObject::Ptr fullData = new DynamicObject();
	fullData->setProperty(mpid::LayoutData, JSON::parse(R"({"StyleSheet": "Dark", "Style": "body\n{\n\tfont-size: 15px;\t\n}\n\n\n\n\n\ntr:focus\n{\n\tbackground: rgba(255,255,255, 0.07);\n}\n\n#footer,\n#header\n{\n\tdisplay: none;\n}\n\n\n\n#content\n{\n\tpadding: 15px;\n\talign-items: flex-start;\n\tbackground: #333;\n\tbox-shadow: inset 0px 0px 5px black;\n\tpadding-top: 24px;\n\t\n}\n\n#content::before\n{\n\tcontent: '';\n\n\tbackground: #222;\n\theight: 20px;\n\tborder-bottom: 1px solid #555;\n}\n\n.header-label\n{\n\tfont-size: 1.1em;\n\tfont-weight: bold;\n\tbackground: linear-gradient(to bottom, #222, #202020);\n\tcolor: #ddd;\n\tborder-radius: 4px;\n\tpadding: 0px;\n}\n\n\n\n.toggle-button\n{\n\tbackground: transparent;\n}\n\n.toggle-button:hover\n{\n\tbackground: transparent;\n}\n\n.tag-button\n{\n\tfont-size: 13px;\n}\n\n.category-button\n{\n\ttransform: none;\n\tpadding: 0px;\n\tcolor: #bbb;\n\twidth: auto;\n\tfont-size: 14px !important;\n\tfont-weight: 500;\n\tborder-radius: 0px;\n\tbackground: #242424;\n\theight: 28px;\n}\n\n.category-button button:hover\n{\n\tcolor: #eee;\n\tbackground: #222;\n}\n\n.category-button:active\n{\n\tcolor: #fff;\n\t\n\ttransform: scale(98%);\n\t\n}\n\n.category-button:checked\n{\n\tbackground: #bbb;\n\tcolor: #222;\n}\n\n.first-child\n{\n\tborder-radius: 50% 0px 50% 0px;\n}\n\n.last-child\n{\n\tborder-radius: 0% 50% 0% 50%;\n}\n\n.category-button::before\n{\n\tdisplay:none;\n}\n\n.category-button::after\n{\n\tdisplay:none;\n}\n\n#searchBar\n{\n\t\n}\n\n#searchBar label\n{\n\tdisplay: none;\n}\n\n#searchBar input\n{\n\tpadding-left: 100vh;\n\theight: 32px;\n}\n\n#searchBar input::before\n{\n\tcontent: '';\n\twidth: 100vh;\n\tbackground-image: \"332.t0lHDd.QGD+iCI159e.QO.jZCkBBSPjNL8yPg4HHDoCS+LjXUJlKDoCS+LzvblCQxSCaCMLm4PD3BG4PhMLm4PTiky4PWq7MDoCLmNzM2RCQDK3qCw1++UDQ4QQzCw1Mc+CQimE2CwFFF7BQ.tptCIVluqBQ7bGvCQX7kPToeO7Pg4HHDU52COjXrnqDDU52COz++c.QIsVqC8+eGPD3BG4Ph8+eGPTgRE4PoB3ADoo3PNDIBd.Q6KGjCwF4P9.Q6KGjCIlzN9.Q0INjCcaiOPzWRE4P213CDAtvQNjX213CDs1gjNDGrbAQ1P7rCElifPjMDO6PhYJ7oPjMDO6P26XLDs1gjNz8NFCQfKbjCI18NFCQrxeeCYJ7oPTYC90Pg4HHDU1feMjX8n2EDU1feMDEM.AQPl.eCo8jOPzAw+3PrIBgGPzAw+3PiUF\";\n\tmargin: 7px;\n\tbackground: #333;\n\t\n}\n\n#snippetList\n{\n\tflex-grow: 1;\n\theight: auto;\n}\n\n\n.description\n{\n\tbackground: #282828;\n\tpadding: 10px;\n}\n\nth\n{\n\tfont-size: 1em;\n}\n\n.setup-page label\n{\n\tmin-width: 0px;\n}\n\n.setup-page button\n{\n\tpadding: 10px 10px;\n}\n\n\n\n.top-button\n{\n\tflex-grow: 0;\n\theight: 32px;\n\twidth: 32px;\n\tbackground: transparent;\n}\n\n.top-button:hover\n{\n\tbackground: transparent;\n}\n\n.top-button::before\n{\n\tborder: 0px;\n\tbackground: #bbb;\n}\n\n#addButton\n{\n\tmargin: 2px;\n}\n\n#addButton::before\n{\n\t\n\tbackground-image: \"320.t010HCBQd5VpCw1kao.Qd5VpCI1yGi.Qd5VpCA.fGPD8da5P..3ADAvsiNDa..3ADALRXNjX..3ADoTHUNzyGi.QfFojCc4VJPDnQJ4PrcMxfPDnQJ4PrcMxfPjbtszPhcMxfPDUeTzPrBgHDgA..MzYjNBQX..PCw1gakBQX..PCIlPuqBQX..PCgwMrPDUeTzPXbCKDImaKMDaXbCKDAZjRNDavQpPDAZjRNjXrfCQDAZjRND..VDQJERkCA.fEQDvHg4PrA.fEQD.2N5PhA.fEQD8da5PrfCQD4oaoNDbjJDQd5VpCwFF2vBQd5VpCwFF2vBQSij0CIFF2vBQIAW1CIz6pPz7+u8PGtUJDM++aODamQ5HDM++aOjXrBgHDM++aOz0HCBQIAW1CcMxfPzzHY8PrcMxfPjmtk5PiUF\";\n\n}\n\n#settingButton::before\n{\n\tbackground-image: \"598.t0lBhtBQ.ADQCIFF7fBQPgePCE+vjPDT3GzPH3UHDADPDMDa3rAHDQytdMjXrRQGDw+7gMjE7nAQb71YC48sWPDui61Pr8XvQPDyQI1PhISLOPDiFt1PvcPCDQLX1Mz.as.QhXSfCwly99.Q5B2hCI1K84.Q1yRjC4SxMPDtUc4P3wZCD45kcNDa..3ADIy+hNjXsM6ADYA3oNDC4g.QHQJrCYLxIPjoCb6ProNgPPjTFZ6Phol9QPD1Pv6PXJ+DDYU.AOj6RYAQFUQwCwlYVRAQtsgzCIl6lcAQzUg0CswgZPDnWj8PoXcGDY8.aODaYgcHDQnKPOjX1tNIDItVQODWTfBQhqUzCg6IqPDgt.8PrAdJuPj0Cr8Ph0NdxPDnWj8PijYMDQWEVOznogCQtsgzCwFIsZCQFUQwCIVdMjCQVEPvCgZA6PD1Pv6P5rGODIkg1NDadcyPDY5.2NjXXbHQDgDovNjhLUDQV.dpCA.fEQjL+K5Pr46T+PjqW14PhAqM+PDtUc4P+JnODYOKQNzWA0CQ5B2hCwFHkFDQhXSfCIVn3+CQDClcC4ry8PDiFt1Pn4yNDwbThMDapfTMDw63tMjXyOrLDwwamMjYq+BQ7OeXCkM4rPDM650ProfnqPDP.QzPi0FA.ZBQBIjhCIlwSwBQBIjhCAtCwPjy3N4Pf6PLDACXeNjXf6PLDA4AqNjwSwBQb3GsCQ.flPDG9Q6PhwDqfPDG9Q6PoD+FDA4AqNTJwuAQv.1mCIVJwuAQNi6jCwDqfPjPBo3PD.nIDIjPJNzXkA\";\n}\n\n#applyButton::before\n{\n\tbackground-image: \"228.t0FXGUBQjNbqCw1Oo3.Q7++1CwF..d.Qx0pyCwFGd5AQBEGnCw12ZwAQBEGnCw12ZwAQ953lCwFGd5AQ953lCwF..d.QJTpVCw1Oo3.QH..PCwFXGUBQ.wiiCwFXGUBQJWahCwFj3dBQJWahCwFj3dBQ.wiiCw1tV6CQH..PCwF..VDQJTpVCwF0g4BQ953lCwVDkBCQ953lCwVDkBCQBEGnCwF0g4BQBEGnCwF..VDQx0pyCw1tV6CQ7++1CwFj3dBQjNbqCwFj3dBQdnjrCwFXGUBQdnjrCwFXGUBQjNbqCMVY\";\n\tmargin: 8px;\n}\n\n#editButton\n{\n\tmargin: 2px;\n\tborder: 0px;\n}\n\n#editButton::before\n{\n\tcontent: '';\n\tbackground-image: \"206.t0VofuCQ19JgCwFYLhAQ2f0xCwlUc9.QanWtCw1kwKCQxLZYCwVofuCQ19JgCMVaVbvPD8c8HMjX..XQDYX1RMD..VDQWHtXCYwACQzoEy1Pr4Eg9PjhP62Pr8Tk0PTTTr0PrwqJ5Pjm9hzPhQaU6PTuRPzP2rNODktbAMDAR5CQoKWPCIVz3.DQoKWPC4jyAQTuRPzPFkuPD4ouHMDaVbvPD8c8HMzXsA.fGPDiFs8ProfqLPDUz86PrwQZUPDgpC8PrA.fGPDiFs8PiUF\";\n\tmargin: 5px;\n}\n\n#closeButton\n{\n\tmargin: 3px;\n}\n\n#closeButton::before\n{\n\tbackground-image: \t\"228.t0FXGUBQjNbqCw1Oo3.Q7++1CwF..d.Qx0pyCwFGd5AQBEGnCw12ZwAQBEGnCw12ZwAQ953lCwFGd5AQ953lCwF..d.QJTpVCw1Oo3.QH..PCwFXGUBQ.wiiCwFXGUBQJWahCwFj3dBQJWahCwFj3dBQ.wiiCw1tV6CQH..PCwF..VDQJTpVCwF0g4BQ953lCwVDkBCQ953lCwVDkBCQBEGnCwF0g4BQBEGnCwF..VDQx0pyCw1tV6CQ7++1CwFj3dBQjNbqCwFj3dBQdnjrCwFXGUBQdnjrCwFXGUBQjNbqCMVY\";\n}\n\n.top-button::before:hover\n{\n\tborder: 0px;\n\tbackground: #eee;\n}\n\n.top-button::after\n{\n\tdisplay: none;\n}\n\n#snippetDirectory label\n{\n\tdisplay: none;\n}\n\n.settings-panel label\n{\n\twidth: 80px;\n}\n\n#newName label\n{\n\tmax-width: 55px;\n\t\n}\n\n.download-box\n{\n\tbackground: #282828; \n\tpadding: 20px; \n\tpadding-top: 10px;\n\tborder-radius: 3px; \n\tborder: 1px solid #444;\n}\n\n.download-box label\n{\n\t\n\twidth: 70px;\n}\n\n.download-button\n{\n\tmargin-top: 30px;\n}\n\n.download-button button::before\n{\n\tbackground-image: \"185.t0lM.ZBQ78+OCIFMbdCQ78+OC4+eEQDrOd2P9+WQDQ8+cNjX9+WQDA8M.ODMbdCQDA.2CYCflPDQ.v8PhgCYUPDQ.v8P9+2ADA8M.Oj++c.QT+emCIl++c.Qv93cCgCYUPDe++yP1.nIDw2++LzXsIWMfPDtVj1PrIWMfPDp.N5PrQegTPDp.N5PrYCflPDSzc7Profd3PDp.N5ProuxrPDp.N5ProuxrPDtVj1PrIWMfPDtVj1PiUF\";\n\tborder: 0px;\n\tbackground-color: #999;\n\tmargin: 3px;\n}\n\n.download-button button::before:hover\n{\n\n\tborder: 0px;\n\tbackground-color: #fff;\n}\n\n.download-button button::after\n{\n\tdisplay: none;\n}\n\n\n#saveFileButton\n{\n\t\n}\n\n#saveFileButton button::after\n{\n\tdisplay: none;\n}\n\n#saveFileButton button::before\n{\n\tbackground-image: \"399.t0F..VDQ.M6eCwF..VDQnUDzCIF..VDQXw0zCg73DQDDRY8Pfx7PDgEfXOjX3UqPDgpqZODn5DDQHb91CAMr+PDBmu8PrguGMPDBmu8PhgKAJPDBmu8P..3ADgY2VOD..d.QPjJzCwF..d.QPqvUCIF..d.Qf3OTCgnFHPDPsrzPvxRBDAI4FMjXf6iBDA+lBMDDyt.QPGCPCg9MMPDzw.zPrATn0PDzw.zPrA.fEQDPy92Pi0F1DbCQfUbnCwFjVWAQfUbnCIFZIQAQfUbnCAwASPDHJQ5PPbvDDAFYmNDaPbvDDAS8SODaHTVNDAS8SODaHTVNDgaglNjXHTVNDg0PkNDNjjCQH4.oCAzr3PD.oL5PhAIP3PD.GI5PPX5MDAVwgND1DbCQfUbnCMVaPETLDAJPQMDaHRWEDAJPQMDaHRWEDAXSzMjXHRWEDAlQ3MD5.YAQfe2dCAxOWPD32s2PrgkfuPD32s2PhAedvPD32s2PPETLDAkV3MDTAECQfJHcCwFTAECQfBTTCMVY\";\n\tborder: 0px;\n\tbackground: #999;\n}\n\n#saveFileButton button::before:hover\n{\n\tbackground: #fff;\n\tborder: 0px;\n}\n\n#clearFilter\n{\n\twidth: 80px !important;\n\t\n}\n\n#clearFilter:disabled\n{\n\topacity: 1.0;\n}\n\n#clearFilter::before\n{\n\tbackground-image:  \"230.t0F++YBQ9..PCIF+adCQ9..PCA.fEQDKPd2P..XQDcA.dNjX..XQDk+M.OD+adCQg++1Cw+elPT3+u8PhM.YUPT3+u8P..3ADk+M.OD..d.QW.fmCIF..d.Qr.4cCM.YUPjO..zP7+mID4C..MzXsQvjrPzE.34PrYcL3PTcBa3PrUsGxPDx3T2Prw+elPzAZG4ProR3ZPDx3T2ProhyTPTcBa3PrwOafPzE.34ProhyTPj98T6ProR3ZPzdiE7Prw+elPDJln5PrUsGxPzdiE7PrYcL3Pj98T6PrQvjrPzE.34PiUF\";\n\tborder: 0px;\n\tbackground-color: #999;\n}\n\n#clearFilter::before:disabled\n{\n\tbackground-color: #444;\n}\n\n#showUserOnly,\n#clearFilter\n{\n\twidth: 90px !important;\n}\n\n#showUserOnly::before\n{\n\tbackground-image: \"451.t0lW.ZBQ.F.PCIFwadCQ.F.PCA.fEQDdOd2P..XQDA..dNjX..XQDQDN.ODwadCQD8+1C4EflPDQ+u8PhoCYUPDQ+u8P..3ADQDN.OD..d.Q..fmCIF..d.Q383cCoCYUPDfA.zPdAnIDAX..MzXs4EflPD.g4zPhgBXWPD.g4zPdhwBDAye+MjmXr.Q..fmCIlmXr.QrADuCgBXWPDgOS8PdAnIDQ3yTOjXTBZMDQ3yTODGnGDQrADuCwA5AQD..34PhwA5AQDL+82PTBZMDAPXNMjW.ZBQ.DlSCMVav2fIDg3fpMjXTmCKDg3fpMD77DCQ3+neCAOOwPDwet3PhAOOwPDj2e4PTmCKDQb+gND7MXBQD2enCIlSg+AQD2enC4t2ZPDj2e4Pt6sFDQ7mKNjXt6sFDg+i9MjSg+AQHNnZCAeClPDhCp1Pi0l8uKAQLmCtCIlUJXAQLJwqCY7QaPDhNj5PbeSHDgnCoNDa1ogJDgnCoNjXVYGLDgnCoNDcFXCQvW+qCAo.4PDr3n6PhAfpzPDtEU7PzodKDg0WLOjOYYBQX8EyCIl1T4AQX8EyCgKOWPDOnQ7P1+tDDwbN3NzXkA\";\n\t\n\tborder: 0px;\n\tbackground-color: #777;\n}\n\n#showUserOnly::before:hover,\n#clearFilter::before:hover\n{\n\tbackground-color: #aaa;\n\tborder: 0px;\n}\n\n#showUserOnly::before:checked\n{\n\tbackground: #fff;\n\tborder: 0px;\n}\n\n#showUserOnly::after,\n#clearFilter::after\n{\n\tdisplay: none;\n}\n\n#showLocation\n{\n\twidth: 24px;\n\n\tflex-grow: 0; \n\theight: 24px;\n}\n\n#showLocation::before\n{\n\tbackground-image: \"348.t0Fq8tCQ37ZrCwFq8tCQT5ssCIFq8tCQfO5wCgw6zPDZ+T8PD0HKDg1OUODavAqEDg1OUOjXT5jCDg1OUOD..d.QfO5wCA.fGPDkda6PrA.fGPD5jr3PhA.fGPDxAS2PT5jCDAChYMDbvZAQvfXVCwFyInAQvfXVCwFyInAQff4dCwFbvZAQff4dCIFSALAQff4dCA.APPDpFO3P.P.DDgNIKNDa.P.DDQo21NjX.P.DDQMO9NDSALAQvcCwCAGrVPDb2P7PrQTirPDb2P7PhAFOvPDb2P7PrlyLDQMO9NDq4LCQT5ssCwFq4LCQ37ZrCwFq8tCQ37ZrCMVaDR6JDw+GTNjX7P1IDA4VVNDkZ3AQDeRmCQnGbPDUK65PhQnGbPDUK65PPQYFDg.e9MDgztBQ3OaeCwFgztBQvDXSCwF..VDQnrxgCwFgztBQDS4oCwFgztBQ7+AkCMVY\";\n\tborder: 0px;\n\tbackground: #999;\n\tmargin: 6px;\n\tmargin-top: 3px;\n}\n\n#showLocation::before:hover\n{\n\tborder: 0px;\n\tbackground: #bbb;\n}\n\n#showLocation::after\n{\n\tdisplay: none;\n}\n\n#sortByPriority label\n{\n\tdisplay: none;\n}", "UseViewport": false, "DialogWidth": 400, "DialogHeight": 900})"));
	fullData->setProperty(mpid::Properties, JSON::parse(R"({"Header": "Header", "Subtitle": "Subtitle", "Image": "", "ProjectName": "SnippetBrowser", "Company": "HISE", "Version": "1.0.0", "BinaryName": "My Binary", "UseGlobalAppData": false, "Icon": ""})"));
	using namespace factory;
	auto mp_ = new Dialog(var(fullData.get()), state, false);
	auto& mp = *mp_;
	auto& List_0 = mp.addPage<List>({
	  { mpid::Style, "gap: 10px; height: 100%;" }
	});

	auto& Column_1 = List_0.addChild<Column>({
	  { mpid::Class, ".header-label" }
	});

	Column_1.addChild<Button>({
	  { mpid::ID, "addButton" }, 
	  { mpid::Code, R"(showAddPage(false);
)" }, 
	  { mpid::Class, ".top-button" }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::Tooltip, "Add snippet to browser" }
	});

	Column_1.addChild<Button>({
	  { mpid::ID, "editButton" }, 
	  { mpid::Code, R"(showAddPage(true);
)" }, 
	  { mpid::Class, ".top-button" }, 
	  { mpid::Style, "display:none;" }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::Tooltip, "Edit the currently loaded snippet" }
	});

	Column_1.addChild<SimpleText>({
	  { mpid::Text, "Snippet Browser" }, 
	  { mpid::Style, "flex-grow: 1" }
	});

	Column_1.addChild<Button>({
	  { mpid::ID, "settingButton" }, 
	  { mpid::Code, "document.navigate(1);" }, 
	  { mpid::Class, ".top-button" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::Tooltip, "Show the snippet browser settings" }
	});

	auto& List_6 = List_0.addChild<List>({
	  { mpid::Style, "display: none;" }
	});

	List_6.addChild<PersistentSettings>({
	  { mpid::ID, "snippetBrowser" }, 
	  { mpid::Filename, "snippetBrowser" }, 
	  { mpid::UseChildState, 1 }, 
	  { mpid::Items, R"(snippetDirectory:""
author: "")" }, 
	  { mpid::UseProject, 0 }, 
	  { mpid::ParseJSON, 0 }
	});

	List_6.addChild<DirectoryScanner>({
	  { mpid::Source, "$snippetDirectory" }, 
	  { mpid::ID, "snippetRoot" }, 
	  { mpid::RelativePath, 1 }, 
	  { mpid::Wildcard, "*.md" }, 
	  { mpid::Directory, 0 }
	});

	auto& Column_9 = List_0.addChild<Column>({
	});

	Column_9.addChild<TextInput>({
	  { mpid::ID, "searchBar" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::Height, 80 }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::CallOnTyping, 1 }, 
	  { mpid::NoLabel, 0 }, 
	  { mpid::Autofocus, 0 }
	});

	Column_9.addChild<Button>({
	  { mpid::ID, "clearFilter" }, 
	  { mpid::Enabled, 0 }, 
	  { mpid::Code, "clearFilter();" }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::Tooltip, "Clear all filters" }, 
	  { mpid::UseOnValue, 1 }
	});

	Column_9.addChild<Button>({
	  { mpid::ID, "showUserOnly" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::Tooltip, "Show only user generated snippets" }
	});

	auto& Column_13 = List_0.addChild<Column>({
	  { mpid::Style, "gap:1px;height: auto;" }
	});

	Column_13.addChild<Button>({
	  { mpid::Text, "All" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::InitValue, "0" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::Class, ".category-button .first-child" }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::Tooltip, "Shows uncategorized / all snippets" }
	});

	Column_13.addChild<Button>({
	  { mpid::Text, "Modules" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::Tooltip, "Show snippets that demonstrate HISE modules" }
	});

	Column_13.addChild<Button>({
	  { mpid::Text, "MIDI" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::Tooltip, "Show snippets related to MIDI processing" }
	});

	Column_13.addChild<Button>({
	  { mpid::Text, "Scripting" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::Tooltip, "Show scripting snippets" }
	});

	Column_13.addChild<Button>({
	  { mpid::Text, "Scriptnode" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::Tooltip, "Show DSP snippets using scriptnode and Faust / SNEX" }
	});

	Column_13.addChild<Button>({
	  { mpid::Text, "UI" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::Class, ".category-button .last-child" }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::Tooltip, "Show snippets related to UI design" }
	});

	auto& List_20 = List_0.addChild<List>({
	  { mpid::Text, "Tags" }, 
	  { mpid::Foldable, 1 }, 
	  { mpid::Folded, 1 }
	});

	List_20.addChild<TagList>({
	  { mpid::ID, "tagList" }, 
	  { mpid::Items, R"(API
Best Practice
Broadcaster
Complex
Faust
Featured
First contact
Full Project
Framework
Graphics API
Helper Tools
Host Sync
Internal Messaging
LookAndFeel
MIDI Player
ScriptPanel
Simple
SNEX
UI Logic)" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::UseOnValue, 1 }
	});

	List_0.addChild<Table>({
	  { mpid::ValueMode, "Row" }, 
	  { mpid::ID, "snippetList" }, 
	  { mpid::Columns, R"(name:Name;max-width:-1;
name:Author;max-width: 140px;width: 110px;)" }, 
	  { mpid::FilterFunction, "showItem" }
	});

	List_0.addChild<MarkdownText>({
	  { mpid::Text, "Please double click a snippet to load..." }, 
	  { mpid::ID, "descriptionDisplay" }, 
	  { mpid::Class, ".description" }, 
	  { mpid::Style, "font-size: 18px; height: 180px;" }
	});

	List_0.addChild<JavascriptFunction>({
	  { mpid::ID, "JavascriptFunctionId" }, 
	  { mpid::Code, R"(var CATEGORIES = ["All", "Modules", "MIDI", "Scripting", "Scriptnode", "UI"];

parsedData = [];

var cf = document.getElementById("clearFilter");

var rebuildTable = function()
{
	var hasFilter = state.category != 0;
	hasFilter = hasFilter || state.searchBar.length != 0;
	hasFilter = hasFilter || state.tagList.length != 0;
	hasFilter = hasFilter || state.showUserOnly;

	Console.print(hasFilter ? "HAS FILTER" : "HAS NO FILTER");

	cf.setAttribute("disabled", !hasFilter);
	cf.updateElement();
	table.updateElement();
};

var clearFilter = function()
{
	state.category = 0;
	state.searchBar = "";
	state.tagList = [];
	state.showUserOnly = false;

	document.navigate(0, false);

	Console.print("CLEAR FILTER");
};

var showAddPage = function(editCurrentSnippet)
{
	state.saveFileButton = false;
	
	if(!editCurrentSnippet)
	{
		state.newName = "";
		state.description = "";
		state.addCategory = 0;
		state.addTagList = [];
	}
	else if (currentlyLoadedData != 0)
	{
		var catValue = CATEGORIES.indexOf(currentlyLoadedData.category);
		
		Console.print("CAT: " + catValue);
		
		state.newName = currentlyLoadedData.name;
		state.addCategory = catValue;
		state.addTagList = currentlyLoadedData.tags.clone();
		
		Console.print("TAGS: " + trace(currentlyLoadedData.tags.clone()));
		
		state.description = currentlyLoadedData.description;
	}
	
	
	document.navigate(2, false);
};

var showItem = function(index, data)
{
	var name = data[0];

	if(state.category != 0)
	{
		var cc = CATEGORIES[state.category];
		var tc = parsedData[index].category;
		
		if(cc != tc)
			return false;
	}
	
	var tagFound = state.tagList.length == 0;
	
	for(i = 0; i < state.tagList.length; i++)
	{
		var tt = parsedData[index].tags;
		
		if(tt.indexOf(state.tagList[i]) != -1)
			tagFound = true;
	}
	
	if(!tagFound)
		return false;
	
	if(state.searchBar.length > 0)
	{
		if(name.toLowerCase().indexOf(state.searchBar.toLowerCase()) == -1)
			return false;
	}
	
	if(state.showUserOnly && state.author != parsedData[index].author)
	{
		return false;
	}
	
	return true;
};

var hiddenItems = ["README.md", "LICENSE.md", "_Template.md", "SnippetDB.md"];

var tags = [];

for(i = 0; i < state.snippetRoot.length; i++)
{
	var item = state.snippetRoot[i];

	if(hiddenItems.indexOf(item) != -1)
		continue;

	var content = document.readFile(state.snippetDirectory + "/" + state.snippetRoot[i]);
	content = content.substring(3, 100000);
	var end = content.indexOf("---");
	var header = content.substring(0, end);
	var description = content.substring(end + 4, 100000);
	var ml = header.split("\n");
	var data = {};
	
	data["name"] = item.split(".")[0];
	data["description"] = description;
	data.tags = [];
	
	for(j = 0; j < ml.length; j++)
	{
		var kv = ml[j].split(":");
		var key = kv[0];
		
		if(key == undefined)
			continue;
			
		if(key == "tags")
		{
			var tagList = kv[1].split(",");
			
			for(t = 0; t < tagList.length; t++)
			{
				var tt = tagList[t].trim();
				
				if(tt.length > 0)
					data.tags.push(tt);
			}
		}
		else
		{
			if(key.length > 0 && kv.length > 1)
				data[key] = kv[1].trim();
		}
	}
	
	for(j = 0; j < data.tags.length; j++)
	{
		if(tags.indexOf(data.tags[j]) == -1)
			tags.push(data.tags[j]);
	}
	
	parsedData.push(data);
}

if(state.sortByPriority)
{
	parsedData.sort(function(d1, d2)
	{
		var p1 = d1.priority ? d1.priority : 3;
		var p2 = d2.priority ? d2.priority : 3;
		
		if(p1 > p2)
			return -1;
		else if(p1 < p2)
			return 1;
		else
			return 0;
	});
}





var tl = document.getElementById("tagList");


var table = document.getElementById("snippetList");

var newItems = "";

for(i = 0; i < parsedData.length; i++)
{
	var item = parsedData[i];

	if(hiddenItems.indexOf(item.name) != -1)
		continue;

	newItems += item.name + "| " + item.author + " |";
	
	if(i != state.snippetRoot.length - 1)
		newItems += "\n";	
}

table.setAttribute("items", newItems);

table.addEventListener("select", function()
{
	var data = parsedData[this.originalRow];
	description.innerText = data.description;
});

var loadSnippet = document.bindCallback("loadSnippet", function(fullPath, category)
{
	Console.print("LOAD SNIPPET: " + fullPath + " from category " + category);
});

var currentlyLoadedData = 0;

var loadFunction = function()
{
	var data = parsedData[this.originalRow];
	currentlyLoadedData = data;
	
	document.getElementById("editButton").style.display = "flex";
	
	
	loadSnippet(data.HiseSnippet, data.category);
};

table.addEventListener("dblclick", loadFunction);
table.addEventListener("keydown", loadFunction);

var description = document.getElementById("descriptionDisplay");
description.innerText = "Please double click a snippet to load...";

if(state.snippetDirectory.length == 0)
{
	document.navigate(1);
}

var editButton = document.getElementById("editButton");
editButton.style.display = "none";

document.clearEventListeners("mainEvents");

document.addEventListener("save", function()
{
	table.setAttribute("items", "");
});

rebuildTable();
)" }
	});

	List_0.addChild<TextInput>({
	  { mpid::ID, "author" }, 
	  { mpid::Style, "display: none;" }, 
	  { mpid::NoLabel, 0 }, 
	  { mpid::Height, 80 }, 
	  { mpid::Autofocus, 0 }, 
	  { mpid::CallOnTyping, 0 }
	});

	// Custom callback for page List_0
	List_0.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_26 = mp.addPage<List>({
	  { mpid::Style, "gap: 10px; " }, 
	  { mpid::Class, ".setup-page" }
	});

	auto& Column_27 = List_26.addChild<Column>({
	  { mpid::Class, ".header-label" }
	});

	Column_27.addChild<SimpleText>({
	  { mpid::Text, "Snippet Browser" }, 
	  { mpid::Class, "#headerlabel" }, 
	  { mpid::Style, "flex-grow: 1;" }
	});

	Column_27.addChild<Button>({
	  { mpid::ID, "applyButton" }, 
	  { mpid::Code, "document.navigate(0, false);" }, 
	  { mpid::Class, ".top-button" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }
	});

	List_26.addChild<MarkdownText>({
	  { mpid::Text, R"(Setup the snippet directory and download the demo assets & snippet data.

)" }
	});

	List_26.addChild<PersistentSettings>({
	  { mpid::ID, "snippetBrowser" }, 
	  { mpid::Filename, "snippetBrowser" }, 
	  { mpid::UseChildState, 1 }, 
	  { mpid::Items, R"(snippetDirectory:""
author: "")" }, 
	  { mpid::UseProject, 0 }, 
	  { mpid::ParseJSON, 0 }
	});

	auto& List_32 = List_26.addChild<List>({
	  { mpid::Style, "background: #282828; padding: 20px; border-radius: 3px; border: 1px solid #444;" }, 
	  { mpid::Class, ".settings-panel" }
	});

	auto& Column_33 = List_32.addChild<Column>({
	});

	Column_33.addChild<SimpleText>({
	  { mpid::Text, "Snippet Directory" }, 
	  { mpid::Style, "text-align: left; width: auto;" }
	});

	Column_33.addChild<Button>({
	  { mpid::ID, "showLocation" }, 
	  { mpid::Code, "Console.print(value);" }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::Tooltip, "Open the snippet directory in the OS file browser" }, 
	  { mpid::UseOnValue, 1 }
	});

	List_32.addChild<FileSelector>({
	  { mpid::Text, "Directory" }, 
	  { mpid::ID, "snippetDirectory" }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "The location where you want to store the snippets." }, 
	  { mpid::Directory, 1 }, 
	  { mpid::SaveFile, 0 }, 
	  { mpid::NoLabel, 0 }
	});

	List_32.addChild<SimpleText>({
	  { mpid::Text, "Username" }, 
	  { mpid::Style, "text-align: left; width: 100%;margin-top: 10px;" }
	});

	List_32.addChild<TextInput>({
	  { mpid::ID, "author" }, 
	  { mpid::Style, "width: 100%;" }, 
	  { mpid::NoLabel, 0 }, 
	  { mpid::Height, 80 }, 
	  { mpid::Help, "The name that is used when you create snippets. Make it your HISE user forum name for increased karma!" }, 
	  { mpid::Autofocus, 0 }, 
	  { mpid::CallOnTyping, 0 }
	});

	List_32.addChild<SimpleText>({
	  { mpid::Text, "Snippet sorting type" }, 
	  { mpid::Style, "text-align: left; width: 100%;margin-top: 10px;" }
	});

	List_32.addChild<Choice>({
	  { mpid::ID, "sortByPriority" }, 
	  { mpid::InitValue, "1" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::NoLabel, 0 }, 
	  { mpid::Custom, 0 }, 
	  { mpid::ValueMode, "Index" }, 
	  { mpid::Help, "Whether to sort the snippets in the list based on their priority or alphabetically" }, 
	  { mpid::Items, R"(Alphabetically
By Priority
)" }
	});

	auto& List_41 = List_26.addChild<List>({
	  { mpid::Class, ".download-box" }
	});

	List_41.addChild<MarkdownText>({
	  { mpid::Text, "**Download example content**" }, 
	  { mpid::Style, "margin: 0px;" }
	});

	List_41.addChild<Button>({
	  { mpid::Text, "Snippets" }, 
	  { mpid::ID, "downloadSnippets" }, 
	  { mpid::InitValue, "true" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::Style, "margin-top: 20px;" }, 
	  { mpid::Help, "Untick this if you don't want to download the snippets (most likely because you've setup the target folder as Git repository already)." }, 
	  { mpid::NoLabel, 0 }
	});

	List_41.addChild<DownloadTask>({
	  { mpid::Text, "Download" }, 
	  { mpid::ID, "downloadSnippets" }, 
	  { mpid::EventTrigger, "OnSubmit" }, 
	  { mpid::Source, "https://github.com/qdr/HiseSnippetDB/archive/refs/heads/main.zip" }, 
	  { mpid::Target, "$snippetDirectory/snippets.zip" }, 
	  { mpid::UsePost, 0 }
	});

	List_41.addChild<UnzipTask>({
	  { mpid::Text, "Extract" }, 
	  { mpid::ID, "extractSnippets" }, 
	  { mpid::EventTrigger, "OnSubmit" }, 
	  { mpid::Overwrite, 1 }, 
	  { mpid::Source, "$snippetDirectory/snippets.zip" }, 
	  { mpid::Style, "margin-bottom: 40px;" }, 
	  { mpid::Target, "$snippetDirectory" }, 
	  { mpid::Cleanup, 1 }, 
	  { mpid::SkipFirstFolder, 1 }, 
	  { mpid::SkipIfNoSource, 1 }
	});

	List_41.addChild<Button>({
	  { mpid::Text, "Assets" }, 
	  { mpid::ID, "downloadAssets" }, 
	  { mpid::InitValue, "true" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::Help, "Untick this if you don't want to download the example assets (about 50MB)" }
	});

	List_41.addChild<DownloadTask>({
	  { mpid::Text, "Download" }, 
	  { mpid::ID, "downloadAssets" }, 
	  { mpid::EventTrigger, "OnSubmit" }, 
	  { mpid::Source, "https://github.com/qdr/HiseSnippetDB/releases/download/1.0.0/Assets.zip" }, 
	  { mpid::Target, "$snippetDirectory/assets.zip" }, 
	  { mpid::UsePost, 0 }
	});

	List_41.addChild<UnzipTask>({
	  { mpid::Text, "Extract" }, 
	  { mpid::ID, "extractAssets" }, 
	  { mpid::EventTrigger, "OnSubmit" }, 
	  { mpid::Overwrite, 1 }, 
	  { mpid::Source, "$snippetDirectory/assets.zip" }, 
	  { mpid::Target, "$snippetDirectory/Assets" }, 
	  { mpid::Cleanup, 1 }, 
	  { mpid::SkipIfNoSource, 1 }, 
	  { mpid::SkipFirstFolder, 0 }
	});

	List_41.addChild<Button>({
	  { mpid::Text, "Download" }, 
	  { mpid::ID, "ButtonId" }, 
	  { mpid::Code, "document.navigate(0, true);" }, 
	  { mpid::Class, ".download-button" }, 
	  { mpid::NoLabel, 0 }, 
	  { mpid::UseOnValue, 1 }
	});

	// Custom callback for page List_26
	List_26.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_50 = mp.addPage<List>({
	  { mpid::Style, "height: 100%;" }
	});

	auto& Column_51 = List_50.addChild<Column>({
	  { mpid::Class, ".header-label" }
	});

	Column_51.addChild<SimpleText>({
	  { mpid::Text, "Add Snippet" }, 
	  { mpid::ID, "editTitle" }, 
	  { mpid::Style, "flex-grow: 1;" }
	});

	Column_51.addChild<Button>({
	  { mpid::ID, "closeButton" }, 
	  { mpid::Code, R"(document.navigate(0, false);
)" }, 
	  { mpid::Class, ".top-button" }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::UseOnValue, 1 }
	});

	auto& List_54 = List_50.addChild<List>({
	  { mpid::Style, "border: 1px solid #222; padding: 15px; gap: 15px; flex-grow: 1;" }
	});

	List_54.addChild<TextInput>({
	  { mpid::Text, "Filename" }, 
	  { mpid::ID, "newName" }, 
	  { mpid::NoLabel, 0 }, 
	  { mpid::Required, 1 }, 
	  { mpid::Height, 80 }, 
	  { mpid::Help, R"(Select the filename for the snippet. It will save a markdown file with the snippet data and the supplied metadata.
)" }, 
	  { mpid::Autofocus, 0 }, 
	  { mpid::CallOnTyping, 0 }
	});

	List_54.addChild<MarkdownText>({
	  { mpid::Text, "Please select a category and tags for the snippet" }
	});

	auto& Column_57 = List_54.addChild<Column>({
	  { mpid::Style, "gap:1px;height: auto;" }
	});

	Column_57.addChild<Button>({
	  { mpid::Text, "All" }, 
	  { mpid::ID, "addCategory" }, 
	  { mpid::InitValue, "0" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::Class, ".category-button .first-child" }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::NoLabel, 1 }
	});

	Column_57.addChild<Button>({
	  { mpid::Text, "Modules" }, 
	  { mpid::ID, "addCategory" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::NoLabel, 1 }
	});

	Column_57.addChild<Button>({
	  { mpid::Text, "MIDI" }, 
	  { mpid::ID, "addCategory" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::NoLabel, 1 }
	});

	Column_57.addChild<Button>({
	  { mpid::Text, "Scripting" }, 
	  { mpid::ID, "addCategory" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::NoLabel, 1 }
	});

	Column_57.addChild<Button>({
	  { mpid::Text, "Scriptnode" }, 
	  { mpid::ID, "addCategory" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::NoLabel, 1 }
	});

	Column_57.addChild<Button>({
	  { mpid::Text, "UI" }, 
	  { mpid::ID, "addCategory" }, 
	  { mpid::Class, ".category-button .last-child" }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::NoLabel, 1 }
	});

	auto& List_64 = List_54.addChild<List>({
	  { mpid::Text, "Tags" }, 
	  { mpid::Foldable, 1 }, 
	  { mpid::Folded, 1 }
	});

	List_64.addChild<TagList>({
	  { mpid::ID, "addTagList" }, 
	  { mpid::Items, R"(API
Best Practice
Broadcaster
Complex
Faust
Featured
First contact
Full Project
Framework
Graphics API
Helper Tools
Host Sync
Internal Messaging
LookAndFeel
MIDI Player
ScriptPanel
Simple
SNEX
UI Logic)" }
	});

	List_54.addChild<Choice>({
	  { mpid::Text, "Priority" }, 
	  { mpid::ID, "priority" }, 
	  { mpid::InitValue, "3" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::NoLabel, 0 }, 
	  { mpid::Custom, 0 }, 
	  { mpid::ValueMode, "ID" }, 
	  { mpid::Help, "This will be used to sort the snippets in the browser. Pick a value between 1 (low priority for edge case snippets) to 5 for the most important examples that should appear at the top." }, 
	  { mpid::Items, R"(1 - low priority
2
3
4
5 - great stuff)" }
	});

	List_54.addChild<SimpleText>({
	  { mpid::Text, "Description" }, 
	  { mpid::Style, "width: 100%; text-align: left;" }
	});

	List_54.addChild<TextInput>({
	  { mpid::ID, "description" }, 
	  { mpid::Style, "flex-grow: 2;  font-family: monospace;vertical-align: top; font-size: 12px; padding-top: 8px; " }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::Required, 1 }, 
	  { mpid::Height, 80 }, 
	  { mpid::Multiline, 1 }, 
	  { mpid::Autofocus, 0 }, 
	  { mpid::CallOnTyping, 1 }
	});

	List_54.addChild<SimpleText>({
	  { mpid::Text, "Preview" }, 
	  { mpid::Style, "width: 100%; text-align: left;" }
	});

	List_54.addChild<MarkdownText>({
	  { mpid::ID, "descriptionPreview" }, 
	  { mpid::Class, ".description" }, 
	  { mpid::Style, "font-size: 18px; height: 180px;" }
	});

	List_54.addChild<Button>({
	  { mpid::Text, "Save file" }, 
	  { mpid::ID, "saveFileButton" }, 
	  { mpid::Code, "document.navigate(0, true);" }, 
	  { mpid::InitValue, "false" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::NoLabel, 0 }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::UseOnValue, 1 }
	});

	List_54.addChild<JavascriptFunction>({
	  { mpid::Code, R"(
document.getElementById("editTitle").innerText = state.newName.length > 0 ? "Edit Snippet" : "Add Snippet";

document.getElementById("descriptionPreview").innerText = "";
document.getElementById("description").addEventListener("change", function()
{
	Console.print("VALUE: " + this.value);
	

	document.getElementById("descriptionPreview").innerText = this.value;
});

)" }
	});

	List_50.addChild<JavascriptFunction>({
	  { mpid::EventTrigger, "OnSubmit" }, 
	  { mpid::Code, R"(function appendLine(key, value)
{
	md += "" + key + ": ";
	
	if(typeof(value) == "object")
	{
		for(i = 0; i < value.length; i++)
		{
			md += value[i];
			if(i != value.length -1)
			   md += ", ";
		}
		
		md += "\n";
	}
	else
	{
		md += value + "\n";
	}
}

var exportSnippet = document.bindCallback("exportSnippet", function()
{
	return "This will be the snippet";
});

if(state.newName.length > 0)
{
	Console.print("SAVE");
	
	// Write the metadata
	var md = "";
	
	md += "---\n";
	
	var CATEGORIES = ["All", "Modules", "MIDI", "Scripting", "Scriptnode", "UI"];
		
	appendLine("author", state.author);
	appendLine("category", CATEGORIES[state.addCategory]);
	appendLine("tags", state.addTagList);
	appendLine("active", "true");
	appendLine("priority", state.priority);
	appendLine("HiseSnippet", exportSnippet());
	
	md += "---\n";
	
	// Write the file
	var fileContent = md;
	fileContent += "\n" + state.description;
	var newFile = state.snippetDirectory + "/" + state.newName.trim() + ".md";
	document.writeFile(newFile, fileContent);
	
}


)" }
	});

	// Custom callback for page List_50
	List_50.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	return mp_;
}
} // namespace library
} // namespace multipage
} // namespace hise