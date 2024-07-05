# Updating HISE


### Develop Branch - WITH FAUST

1. Sync the latest changes from the original HISE to your fork ("develop" for Mac, "develop-win" for Win).

2. Copy the contents of `/tools/SDK` and `/tools/faust/` from the old folder to the new one.

3. Open `/projects/standalone/HISE Standalone.jucer`.

4. ON MAC:
	In the `hi_core` module, make sure the `USE_IPP` flag is set to *FALSE*.
	In the `Xcode (macOS)` exporter tab, add `NUM_HARDCODED_FX_MODS=6` and `NUM_HARDCODED_POLY_FX_MODS=6` to Extra Preprocessor Definitions.
	In the `Xcode (macOS)` exporter tab, scroll to `Valid Architectures`, and make sure only `x86_64` is checked.

   ON WIN:
   	In the `hi_core` module, make sure the `USE_IPP` flag is set to *TRUE*.
   	In the `Visual Studio 2022` exporter tab, add `NUM_HARDCODED_FX_MODS=6` and `NUM_HARDCODED_POLY_FX_MODS=6` to Extra Preprocessor Definitions.
   	In the `Release with Faust` tab, add "C:\Program Files (x86)\Intel\oneAPI\ipp\2021.10\include" to Header Search Paths. (NOTE: You'll have to do that when exporting plugins too.)

   ON BOTH:
   	In the `hi_faust` module, make sure `HISE_INCLUDE_FAUST` is enabled.
   	In the `hi_faust_types` module, if you're using a faust version older than 2.54.0 (which I am as of 9/3/23), also enable `FAUST_NO_WARNING_MESSAGES`.

5. Click `Save and Open in IDE`.

6. ON MAC:
	Click Product > Scheme > Edit Scheme.
	Choose `Release with Faust`. Make sure `Debug Executable` is checked.
	Click Product > Build For > Running.

   ON WIN:
   	Choose `Release with Faust` in the dropdown in the top bar.
   	Click Build > Build Solution.



*NOTE* If you need to compile HISE with Faust as a plugin, the plugin Projucer file is not set up correctly for that. Open the standalone Projucer file and add all the Faust-related settings, flags, etc. to the plugin version. Then in Xcode, choose Product > Scheme > `HISE - All`, then Product > Build For > Profiling.




