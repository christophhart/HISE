echo off

del "xml\selection" /Q
rd "xml\selection"

doxygen "xml.doxyfile"


md "xml\selection"

xcopy "xml\class_scripting_api_1_1_console.xml" "xml\selection"
xcopy "xml\class_scripting_api_1_1_content.xml" "xml\selection"
xcopy "xml\class_scripting_api_1_1_engine.xml" "xml\selection"
xcopy "xml\class_scripting_api_1_1_message.xml" "xml\selection"
xcopy "xml\class_scripting_api_1_1_synth.xml" "xml\selection"
xcopy "xml\class_scripting_api_1_1_sampler.xml" "xml\selection"
xcopy "xml\class_scripting_api_1_1_math.xml" "xml\selection"
xcopy "xml\class_scripting_objects_1_1_scripting_modulator.xml" "xml\selection"
xcopy "xml\class_scripting_objects_1_1_midi_list.xml" "xml\selection"
xcopy "xml\class_scripting_objects_1_1_scripting_audio_sample_processor.xml" "xml\selection"
xcopy "xml\class_scripting_objects_1_1_scripting_table_processor.xml" "xml\selection"
xcopy "xml\class_scripting_objects_1_1_scripting_effect.xml" "xml\selection"
xcopy "xml\class_scripting_objects_1_1_scripting_midi_processor.xml" "xml\selection"
xcopy "xml\class_scripting_objects_1_1_scripting_preset_storage.xml" "xml\selection"

xcopy "xml\struct_scripting_api_1_1_content_1_1_script_button.xml" "xml\selection"
xcopy "xml\struct_scripting_api_1_1_content_1_1_script_combo_box.xml" "xml\selection"
xcopy "xml\struct_scripting_api_1_1_content_1_1_script_slider.xml" "xml\selection"
xcopy "xml\struct_scripting_api_1_1_content_1_1_script_label.xml" "xml\selection"
xcopy "xml\struct_scripting_api_1_1_content_1_1_modulator_meter.xml" "xml\selection"
xcopy "xml\struct_scripting_api_1_1_content_1_1_script_table.xml" "xml\selection"
xcopy "xml\struct_scripting_api_1_1_content_1_1_scripted_plotter.xml" "xml\selection"
xcopy "xml\struct_scripting_api_1_1_content_1_1_script_image.xml" "xml\selection"
xcopy "xml\struct_scripting_api_1_1_content_1_1_script_slider_pack.xml" "xml\selection"


ren "xml\selection\class_scripting_api_1_1_console.xml" "Console.xml"
ren "xml\selection\class_scripting_api_1_1_content.xml" "Content.xml"
ren "xml\selection\class_scripting_api_1_1_engine.xml" "Engine.xml"
ren "xml\selection\class_scripting_api_1_1_message.xml" "Message.xml"
ren "xml\selection\class_scripting_api_1_1_synth.xml" "Synth.xml"
ren "xml\selection\class_scripting_api_1_1_sampler.xml" "Sampler.xml"
ren "xml\selection\class_scripting_api_1_1_math.xml" "Math.xml"



ren "xml\selection\class_scripting_objects_1_1_scripting_modulator.xml" "Modulator.xml"
ren "xml\selection\class_scripting_objects_1_1_midi_list.xml" "MidiList.xml"
ren "xml\selection\class_scripting_objects_1_1_scripting_effect.xml" "Effect.xml"
ren "xml\selection\class_scripting_objects_1_1_scripting_midi_processor.xml" "MidiProcessor.xml"
ren "xml\selection\class_scripting_objects_1_1_scripting_audio_sample_processor.xml" "AudioSampleProcessor.xml"
ren "xml\selection\class_scripting_objects_1_1_scripting_table_processor.xml" "TableProcessor.xml"
ren "xml\selection\class_scripting_objects_1_1_scripting_synth.xml" "ChildSynth.xml"
ren "xml\selection\class_scripting_objects_1_1_scripting_preset_storage.xml" "PresetStorage.xml"

ren "xml\selection\struct_scripting_api_1_1_content_1_1_script_button.xml" "ScriptButton.xml"
ren "xml\selection\struct_scripting_api_1_1_content_1_1_script_combo_box.xml" "ScriptComboBox.xml"
ren "xml\selection\struct_scripting_api_1_1_content_1_1_script_slider.xml" "ScriptSlider.xml"
ren "xml\selection\struct_scripting_api_1_1_content_1_1_script_label.xml" "ScriptLabel.xml"
ren "xml\selection\struct_scripting_api_1_1_content_1_1_modulator_meter.xml" "ModulatorMeter.xml"
ren "xml\selection\struct_scripting_api_1_1_content_1_1_script_table.xml" "ScriptTable.xml"
ren "xml\selection\struct_scripting_api_1_1_content_1_1_script_image.xml" "ScriptImage.xml"
ren "xml\selection\struct_scripting_api_1_1_content_1_1_scripted_plotter.xml" "ScriptedPlotter.xml"
ren "xml\selection\struct_scripting_api_1_1_content_1_1_script_slider_pack.xml" "ScriptSliderPack.xml"

ApiExtractor.exe xml\selection xml\selection


del xml\selection\*.xml /Q

BinaryBuilder.exe xml\selection "..\..\hi_scripting\scripting\api" XmlApi

del "xml\selection" /Q
REM #rd "xml\selection"

:END
