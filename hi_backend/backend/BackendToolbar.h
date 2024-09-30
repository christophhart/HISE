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

#ifndef BACKENDTOOLBAR_H_INCLUDED
#define BACKENDTOOLBAR_H_INCLUDED

namespace hise { using namespace juce;

class BackendProcessorEditor;


class MainToolbarFactory: public PathFactory
{
public:

	Path createPath(const String& id) const override;
	String getId() const override { return "Main Toolbar"; }

private:

	BackendProcessorEditor *editor;

	

};

class ImporterBase: public hlac::HlacArchiver::Listener
{
public:

	ImporterBase(BackendRootWindow* bpe_):
	  bpe(bpe_),
	  ok(Result::ok())
	{}

protected:
	
	~ImporterBase() {};

	void logStatusMessage(const String& message) override;
	void logVerboseMessage(const String& verboseMessage) override;;
	void criticalErrorOccured(const String& message) override;

	virtual File getProjectFolder() const = 0;
	virtual void showStatusMessageBase(const String& message) = 0;
	virtual DialogWindowWithBackgroundThread::LogData* getLogData() = 0;
	virtual Thread* getThreadToUse() = 0;
	virtual double& getJobProgress() = 0;

	void createSubDirectories();
	void extractPools();
	void extractFonts();
	void extractNetworks();
	void extractScripts();
	void createProjectSettings();
	void extractPreset();
	void extractUserPresets();
	void extractWebResources();
	void extractHxi(const File& archive);
	void createProjectData();

	template <typename DataType, typename PoolType> void writePool(PoolType& pool, const std::function<void(File, const DataType&, const var&)>& fileOp)
	{
		auto id = pool.getFileTypeName();

		showStatusMessageBase("Extract " + id + " Pool...");

		auto type = FileHandlerBase::getSubDirectoryForIdentifier(id);
		auto poolRoot = e->getRootFolder().getChildFile(e->getIdentifier(type));
		pool.loadAllFilesFromDataProvider();

		for (int i = 0; i < pool.getNumLoadedFiles(); i++)
		{
			if (auto ptr = pool.loadFromReference(pool.getReference(i), PoolHelpers::LoadAndCacheWeak))
			{
				getJobProgress() = (double)i / (double)pool.getNumLoadedFiles();

				auto target = ptr->ref.resolveFile(e, type);

				target.getParentDirectory().createDirectory();
				target.deleteFile();

				fileOp(target, ptr->data, ptr->additionalData);
			}
		}
	}

	BackendRootWindow* bpe;
	ScopedPointer<FullInstrumentExpansion> e = nullptr;
	Result ok;
	Array<File> sampleArchives;
};

} // namespace hise

#endif  // BACKENDTOOLBAR_H_INCLUDED
