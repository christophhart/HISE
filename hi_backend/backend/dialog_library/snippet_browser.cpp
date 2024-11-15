
namespace hise {
namespace multipage {
namespace library
{
var SnippetBrowser::rebuildTable(const var::NativeFunctionArgs& args)
{
	StringArray hiddenItems = {"README.md", "LICENSE.md", "_Template.md", "SnippetDB.md"};

	auto id = args.arguments[0].toString();

	if(id == "tableInitialiser")
	{
		auto snippetRoot = File(readState("snippetDirectory").toString());

		if(!snippetRoot.isDirectory())
		{
			navigate(1, false);
			return var(true);
		}

		writeState("addButton", 0);
		writeState("editButton", 0);

		setElementProperty("editButton", mpid::Visibility, currentlyLoadedData.isObject() ? "Default" : "Placeholder");

		parsedData.clear();

		Array<var> tags;

		

		for(int i = 0; i < readState("snippetRoot").size(); i++)
		{
			auto item = readState("snippetRoot")[i].toString();

			if(hiddenItems.indexOf(item) != -1)
				continue;

			auto content = snippetRoot.getChildFile(item).loadFileAsString();

			content = content.substring(3, 100000);
			auto end = content.indexOf("---");
			auto header = content.substring(0, end);
			auto description = content.substring(end + 4, 100000);
			auto ml = StringArray::fromLines(header);
			DynamicObject::Ptr data = new DynamicObject();
			
			data->setProperty("name", StringArray::fromTokens(item, ".", "")[0]);
			data->setProperty("description", description);
			data->setProperty("tags", var(Array<var>()));
			
			for(int j = 0; j < ml.size(); j++)
			{
				auto kv = StringArray::fromTokens(ml[j], ":", "");
				auto key = kv[0];

				if(key.isEmpty())
					continue;
					
				if(key == "tags")
				{
					auto tagList = StringArray::fromTokens(kv[1], ",", "");
					
					for(int t = 0; t < tagList.size(); t++)
					{
						auto tt = tagList[t].trim();
						
						if(tt.isNotEmpty())
							data->getProperty("tags").getArray()->add(tt);
					}
				}
				else
				{
					if(key.isNotEmpty() && kv.size() > 1)
						data->setProperty(key, kv[1].trim());
				}
			}
			
			for(int j = 0; j < data->getProperty("tags").size(); j++)
			{
				auto thisTag = data->getProperty("tags")[j];
				tags.addIfNotAlreadyThere(thisTag);
			}
			
			parsedData.add(var(data.get()));
		}

		enum Sorting
		{
			Alphabetically,
			Priority,
			MostRecent
		};

		auto sorting = (Sorting)(int)readState("sortByPriority");

		if(sorting != Sorting::Alphabetically)
		{
			struct PrioritySorter
			{
				static int compareElements(const var& d1, const var& d2)
				{
					auto p1 = (int)d1["priority"] ? (int)d1["priority"] : 3;
					auto p2 = (int)d2["priority"] ? (int)d2["priority"] : 3;

					if(p1 > p2)
						return -1;
					else if(p1 < p2)
						return 1;
					else
						return 0;
				}
			};

			struct MostRecentSorter
			{
				static int compareElements(const var& d1, const var& d2)
				{
					auto p1 = d1["date"].toString();
					auto p2 = d2["date"].toString();

					auto t1 = Time::fromISO8601(p1);
					auto t2 = Time::fromISO8601(p2);

					if(t1 > t2)
						return -1;
					else if(t1 < t2)
						return 1;
					else
						return 0;
				}
			};

			if(sorting == Sorting::Priority)
			{
				PrioritySorter sorter;
				parsedData.sort(sorter);
			}
			if(sorting == Sorting::MostRecent)
			{
				MostRecentSorter sorter;
				parsedData.sort(sorter);
			}

			
#if 0
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
#endif
		}
	}

	auto hasFilter = (int)readState("category") != 0;
	hasFilter = hasFilter || readState("searchBar").toString().isNotEmpty();
	hasFilter = hasFilter || readState("tagList").size() != 0;
	hasFilter = hasFilter || (bool)readState("showUserOnly");

	setElementProperty("clearFilter", mpid::Enabled, hasFilter);
	
	String newItems = "";

	for(int i = 0; i < parsedData.size(); i++)
	{
		auto item = parsedData[i];

		if(hiddenItems.indexOf(item["name"].toString()) != -1)
			continue;

		newItems << item["name"].toString() << "| " << item["author"].toString() << " |";
		
		if(i != parsedData.size() - 1)
			newItems << "\n";	
	}

	this->setElementProperty("snippetList", mpid::Items, newItems);

	return var(true);
}

var SnippetBrowser::clearFilter(const var::NativeFunctionArgs& args)
{
	writeState("category", 0);
	writeState("searchBar", "");
	writeState("tagList", Array<var>());
	writeState("showUserOnly", false);

	MessageManager::callAsync([this]()
	{
		dialog->refreshCurrentPage();
	});
		
	return var();
}

var SnippetBrowser::onTable(const var::NativeFunctionArgs& args)
{
	if(auto eventObj = args.arguments[1].getDynamicObject())
	{
		auto eventType = args.arguments[1]["eventType"].toString();
		auto isLoadAction = eventType == "keydown" || eventType == "dblclick";

		auto originalIndex = args.arguments[1]["originalRow"];

		if(isLoadAction)
		{
			setElementProperty("editButton", mpid::Visibility, "Default");

			currentlyLoadedData = parsedData[originalIndex];

			auto snippet = currentlyLoadedData["HiseSnippet"].toString();
			auto category = currentlyLoadedData["category"].toString();

			auto am = bpe->getBackendProcessor()->getAssetManager();
			am->initialise();
			BackendCommandTarget::Actions::loadSnippet(bpe, snippet);
			auto root = bpe->getRootFloatingTile();

			auto catIndex = SnippetBrowserHelpers::getCategoryNames().indexOf(category);

			if(catIndex != -1)
			{
				auto c = (SnippetBrowserHelpers::Category)catIndex;
				auto foldState = hise::SnippetBrowserHelpers::getFoldConfiguration(c);

				root->forEach<FloatingTileContent>([&](FloatingTileContent* t)
				{
					auto s = t->getParentShell()->getLayoutData().getKeyPressId();

					if(s.isValid() && foldState.find(s) != foldState.end())
					{
						auto x = foldState[s];
						t->getParentShell()->setFolded(x);
					}
								
					return false;
				});
			    
				bpe->currentCategory = c;
			}
			    
			bpe->setCurrentlyActiveProcessor();

			loadSnippet(args);
		}
		else
		{
			var data = parsedData[originalIndex];
			auto description = data["description"].toString();

			setElementProperty("descriptionDisplay", mpid::Text, description);
		}
	}

	

	return var();
}

var SnippetBrowser::loadSnippet(const var::NativeFunctionArgs& args)
{
	if(args.numArguments != 2)
		return var();

	
		
	return var();
}

var SnippetBrowser::showItem(const var::NativeFunctionArgs& args)
{
	StringArray CATEGORIES = { "All", "Modules", "MIDI", "Scripting", "Scriptnode", "UI" };
	// name = data[0];

	auto index = (int)args.arguments[0];

	auto thisCategory = (int)readState("category");

	if(thisCategory != 0)
	{
		auto cc = CATEGORIES[thisCategory];
		auto tc = parsedData[index]["category"].toString();
		
		if(cc != tc)
			return false;
	}

	auto tagList = readState("tagList");

	auto tagFound = tagList.size() == 0;
	
	for(int i = 0; i < tagList.size(); i++)
	{
		auto tt = parsedData[index]["tags"];
		
		if(tt.indexOf(tagList[i]) != -1)
			tagFound = true;
	}
	
	if(!tagFound)
		return false;

	auto searchText = readState("searchBar").toString();

	if(searchText.isNotEmpty())
	{
		auto name = parsedData[index]["name"].toString();

		if(name.toLowerCase().indexOf(searchText.toLowerCase()) == -1)
			return false;
	}
	
	if(readState("showUserOnly") && readState("author") != parsedData[index]["author"])
		return false;
	
	return true;
}

var SnippetBrowser::saveSnippet(const var::NativeFunctionArgs& args)
{
	String md;

	auto appendLine = [&](const String& key, const var& value)
	{
		md << "" << key << ": ";
		
		if(value.isArray())
		{
			for(int i = 0; i < value.size(); i++)
			{
				md << value[i].toString();
				if(i != value.size() -1)
				   md << ", ";
			}
			
			md << "\n";
		}
		else
		{
			md << value.toString() << "\n";
		}
	};

	auto newName = readState("newName").toString();

	if(newName.isNotEmpty())
	{
		// Write the metadata
		
		md += "---\n";
		
		StringArray CATEGORIES = { "All", "Modules", "MIDI", "Scripting", "Scriptnode", "UI" };
			
		appendLine("author", readState("author"));
		appendLine("category", CATEGORIES[readState("addCategory")]);
		appendLine("tags", readState("addTagList"));
		appendLine("active", "true");
		appendLine("priority", readState("priority"));
		appendLine("date", Time::getCurrentTime().toISO8601(false));

		auto content = BackendCommandTarget::Actions::exportFileAsSnippet(bpe, false);
        
		appendLine("HiseSnippet", content);
		
		md += "---\n";
		
		// Write the file
		String fileContent = md;
		fileContent << "\n" + readState("description").toString();

		auto newFile = File(readState("snippetDirectory")).getChildFile(readState("newName").toString().trim()).withFileExtension(".md");

		newFile.replaceWithText(fileContent);

		SystemClipboard::copyTextToClipboard("###" + readState("newName").toString().trim() + "\n\n```\n" + fileContent+ "\n```");
	}

	navigate(0, false);

	return var();
}

var SnippetBrowser::initAddPage(const var::NativeFunctionArgs& args)
{
	auto isEdit = (bool)readState("editButton");

	writeState("saveFileButton", 0);

	if(!isEdit)
	{
		writeState("newName", "");
		writeState("description", "");
		writeState("addCategory", 0);
		writeState("addTagList", Array<var>());
		writeState("priority", 0);
		
	}
	else if (currentlyLoadedData.isObject())
	{
		auto d = currentlyLoadedData;

		writeState("newName", d["name"]);
		writeState("description", d["description"]);

		StringArray CATEGORIES = { "All", "Modules", "MIDI", "Scripting", "Scriptnode", "UI" };
		
		writeState("addCategory", CATEGORIES.indexOf(d["category"].toString()));
		writeState("addTagList", d["tags"].clone());
		writeState("priority", d["priority"]);
	}

	MessageManager::callAsync([this, isEdit]()
	{
		setElementProperty("editTitle", mpid::Text, isEdit ? "Edit snippet" : "Add snippet");
		setElementProperty("descriptionPreview", mpid::Text, readState("description"));
	});
	
	return var();
}

var SnippetBrowser::updatePreview(const var::NativeFunctionArgs& args)
{
	setElementProperty("descriptionPreview", mpid::Text, readState("description"));
	return var(true);
}
} // library
} // multipage	 
} // hise
