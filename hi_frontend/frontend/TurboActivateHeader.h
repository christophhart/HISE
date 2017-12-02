/*
  ==============================================================================

    TurboActivateHeader.h
    Created: 7 Oct 2017 12:38:10pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef TURBOACTIVATEHEADER_H_INCLUDED
#define TURBOACTIVATEHEADER_H_INCLUDED


#if USE_TURBO_ACTIVATE


#ifdef _WIN32
typedef std::wstring TurboActivateStringType;
typedef const wchar_t* TurboActivateCharPointerType;
#else
typedef std::string TurboActivateStringType;
typedef const char* TurboActivateCharPointerType;
#endif

class TurboActivateUnlocker
{
public:

	enum class State
	{
		Activated,
		ActivatedButFailedToConnect,
		Deactivated,
		TrialExpired,
		Trial,
		Invalid,
		KeyFileFailedToOpen,
		LicenseFileDoesNotExist,
		NoInternet,
		numStates
	};

	TurboActivateUnlocker(TurboActivateCharPointerType pathToLicenceFile);

	~TurboActivateUnlocker();

	bool isUnlocked() const noexcept
	{
		return unlockState == State::Activated || unlockState == State::ActivatedButFailedToConnect;
	}

	bool licenceWasFound() const noexcept
	{
		return unlockState != State::KeyFileFailedToOpen && unlockState != State::LicenseFileDoesNotExist;
	}

	bool licenceExpired() const noexcept
	{
		return unlockState == State::Deactivated;
	}

	std::string getProductKey() const;

	std::string getErrorMessage() const;

	void activateWithKey(const char* key);

	void activateWithFile(TurboActivateCharPointerType filePath);

	void writeActivationFile(const char* key, TurboActivateCharPointerType fileName);

	void deactivateThisComputer();

	void deactivateWithFile(TurboActivateCharPointerType path);

	void reactivate();

	State unlockState;

	class Pimpl;

	Pimpl *p;


	struct Logger
	{
		Logger(TurboActivateCharPointerType turboActivateFilePath);

		~Logger();

		void logToFile(TurboActivateCharPointerType message);

	private:

        juce::FileOutputStream* fos = nullptr;
	};

};

#endif


#endif  // TURBOACTIVATEHEADER_H_INCLUDED
