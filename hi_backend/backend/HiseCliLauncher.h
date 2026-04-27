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
*   ===========================================================================
*/

#pragma once

namespace hise { using namespace juce;

/** Spawns hise-cli in a new OS-native terminal window. The spawned terminal
    is not owned by HISE -- it stays alive after HISE quits.

    hise-cli is expected to be on PATH on every supported OS.
*/
struct HiseCliLauncher
{
	/** Launches hise-cli in a new terminal at the given working directory.
	    On Linux, linuxOverride may contain a full command template with
	    {cwd} and {cmd} placeholders. If empty, common terminals are probed.
	*/
	static Result launch(const File& cwd, const String& linuxOverride)
	{
		const String cmdName("hise-cli");

	#if JUCE_MAC

		ignoreUnused(linuxOverride);

		String pathEsc = cwd.getFullPathName().replace("\\", "\\\\").replace("\"", "\\\"");

		String doScript;
		doScript << "do script \"cd \\\"" << pathEsc << "\\\" && " << cmdName << "\"";

		StringArray args;
		args.add("osascript");
		args.add("-e"); args.add("tell application \"Terminal\"");
		args.add("-e"); args.add("activate");
		args.add("-e"); args.add(doScript);
		args.add("-e"); args.add("end tell");

		ChildProcess cp;
		if (!cp.start(args))
			return Result::fail("Failed to invoke osascript");

		return Result::ok();

	#elif JUCE_WINDOWS

		ignoreUnused(linuxOverride);

		StringArray args;
		args.add("cmd.exe");
		args.add("/c");
		args.add("start");
		args.add("HISE CLI");
		args.add("/D");
		args.add(cwd.getFullPathName());
		args.add("cmd.exe");
		args.add("/K");
		args.add(cmdName);

		ChildProcess cp;
		if (!cp.start(args))
			return Result::fail("Failed to start console window");

		return Result::ok();

	#elif JUCE_LINUX

		String invocation;

		if (linuxOverride.isNotEmpty())
		{
			invocation = linuxOverride.replace("{cwd}", cwd.getFullPathName().quoted())
			                          .replace("{cmd}", cmdName);
		}
		else
		{
			struct TerminalEntry { const char* name; String (*build)(const File&, const String&); };

			auto build_gnome  = [](const File& d, const String& c) { return "gnome-terminal --working-directory=" + d.getFullPathName().quoted() + " -- " + c; };
			auto build_konsole= [](const File& d, const String& c) { return "konsole --workdir " + d.getFullPathName().quoted() + " -e " + c; };
			auto build_xfce   = [](const File& d, const String& c) { return "xfce4-terminal --working-directory=" + d.getFullPathName().quoted() + " -e " + c; };
			auto build_alac   = [](const File& d, const String& c) { return "alacritty --working-directory " + d.getFullPathName().quoted() + " -e " + c; };
			auto build_kitty  = [](const File& d, const String& c) { return "kitty --directory " + d.getFullPathName().quoted() + " " + c; };
			auto build_xterm  = [](const File& d, const String& c) { return "xterm -e 'cd " + d.getFullPathName() + " && " + c + "'"; };
			auto build_xte    = [](const File& d, const String& c) { return "x-terminal-emulator -e 'cd " + d.getFullPathName() + " && " + c + "'"; };

			const TerminalEntry table[] = {
				{ "gnome-terminal",     build_gnome   },
				{ "konsole",            build_konsole },
				{ "xfce4-terminal",     build_xfce    },
				{ "alacritty",          build_alac    },
				{ "kitty",              build_kitty   },
				{ "xterm",              build_xterm   },
				{ "x-terminal-emulator",build_xte     }
			};

			auto exists = [](const String& name)
			{
				return File("/usr/bin/" + name).existsAsFile()
				    || File("/usr/local/bin/" + name).existsAsFile()
				    || File("/bin/" + name).existsAsFile();
			};

			for (auto& entry : table)
			{
				if (exists(entry.name))
				{
					invocation = entry.build(cwd, cmdName);
					break;
				}
			}

			if (invocation.isEmpty())
				return Result::fail("No terminal emulator found. Set 'Linux Terminal Command' in Settings -> Development.");
		}

		StringArray args;
		args.add("setsid");
		args.add("sh");
		args.add("-c");
		args.add(invocation + " >/dev/null 2>&1 &");

		ChildProcess cp;
		if (!cp.start(args))
			return Result::fail("Failed to spawn terminal");

		return Result::ok();

	#else
		ignoreUnused(cwd, linuxOverride);
		return Result::fail("Unsupported platform");
	#endif
	}
};

} // namespace hise
