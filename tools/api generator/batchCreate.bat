REM echo off

del "xml\selection" /Q
rd "xml\selection"

doxygen "xml.doxyfile"


md "xml\selection"

xcopy "xml\classhise_1_1_scripting_api_1_1_colours.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_console.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_content.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_engine.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_settings.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_math.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_message.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_synth.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_sampler.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_transport_handler.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_threads.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_date.xml" "xml\selection"

xcopy "xml\structhise_1_1fixobj_1_1_array.xml" "xml\selection"
xcopy "xml\structhise_1_1fixobj_1_1_factory.xml" "xml\selection"
xcopy "xml\structhise_1_1fixobj_1_1_stack.xml" "xml\selection"

xcopy "xml\classhise_1_1_scripting_objects_1_1_script_file.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_server.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_api_1_1_file_system.xml" "xml\selection"

xcopy "xml\classhise_1_1_scripting_api_1_1_module_ids.xml" "xml\selection"
xcopy "xml\structhise_1_1_hise_javascript_engine_1_1_root_object_1_1_math_class.xml" "xml\selection"
xcopy "xml\classhise_1_1_doxygen_array_functions.xml" "xml\selection"
xcopy "xml\classhise_1_1_doxygen_string_functions.xml" "xml\selection"


xcopy "xml\classhise_1_1_scripting_objects_1_1_scripted_look_and_feel.xml" "xml\selection"


xcopy "xml\classhise_1_1_script_loris_manager.xml" "xml\selection"

xcopy "xml\classhise_1_1_script_expansion_handler.xml" "xml\selection"
xcopy "xml\classhise_1_1_script_expansion_reference.xml" "xml\selection"

xcopy "xml\classhise_1_1_dsp_instance.xml" "xml\selection"

xcopy "xml\classhise_1_1_scripting_objects_1_1_scripting_modulator.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_scripting_synth.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_midi_list.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_slider_pack_data.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_table_data.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_ring_buffer.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_unordered_stack.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_objects_1_1_script_modulation_matrix.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_shader.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_audio_file.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_scripting_sampler_sound.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_scripting_message_holder.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_scripting_audio_sample_processor.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_scripting_table_processor.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_display_buffer_source.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_slider_pack_processor.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_f_f_t.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_buffer.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_scripted_midi_automation_handler.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_scripted_macro_handler.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_thread_safe_storage.xml" "xml\selection"
xcopy "xml\classhise_1_1_beatport_manager.xml" "xml\selection"



xcopy "xml\classhise_1_1_scripting_objects_1_1_scripting_effect.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_routing_matrix.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_scripting_midi_processor.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_scripting_preset_storage.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_scripting_slot_f_x.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_markdown_object.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_graphics_object.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_path_object.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_script_neural_network.xml" "xml\selection"

xcopy "xml\structhise_1_1_scripting_objects_1_1_global_cable_reference.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_objects_1_1_global_routing_manager_reference.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_objects_1_1_script_broadcaster.xml" "xml\selection"

xcopy "xml\classhise_1_1_scripting_objects_1_1_scripted_midi_player.xml" "xml\selection"
xcopy "xml\classhise_1_1_scripting_objects_1_1_timer_object.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_objects_1_1_script_download_object.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_objects_1_1_script_background_task.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_objects_1_1_script_builder.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_objects_1_1_script_error_handler.xml" "xml\selection"

xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_button.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_combo_box.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_slider.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_label.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_modulator_meter.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_table.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_scripted_plotter.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_image.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_panel.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_web_view.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_slider_pack.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_scripted_viewport.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_floating_tile.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_audio_waveform.xml" "xml\selection"
xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_multipage_dialog.xml" "xml\selection"



xcopy "xml\structhise_1_1_scripting_api_1_1_content_1_1_script_slider_1_1_modifier_object.xml" "xml\selection"

xcopy "xml\classhise_1_1_script_user_preset_handler.xml" "xml\selection"


xcopy "xml\structhise_1_1_script_unlocker_1_1_ref_object.xml" "xml\selection"


xcopy "xml\classscriptnode_1_1_dsp_network.xml" "xml\selection"
xcopy "xml\structscriptnode_1_1_script_network_test.xml" "xml\selection"
xcopy "xml\classscriptnode_1_1_node_base.xml" "xml\selection"
xcopy "xml\classscriptnode_1_1_connection_base.xml" "xml\selection"
xcopy "xml\classscriptnode_1_1_node_base_1_1_parameter.xml" "xml\selection"


ren "xml\selection\classhise_1_1_scripting_api_1_1_colours.xml" "Colours.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_console.xml" "Console.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_content.xml" "Content.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_engine.xml" "Engine.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_settings.xml" "Settings.xml"
ren "xml\selection\structhise_1_1_hise_javascript_engine_1_1_root_object_1_1_math_class.xml" "Math.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_message.xml" "Message.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_synth.xml" "Synth.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_sampler.xml" "Sampler.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_transport_handler.xml" "TransportHandler.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_threads.xml" "Threads.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_module_ids.xml" "ModuleIds.xml"
ren "xml\selection\classhise_1_1_dsp_instance.xml" "DspModule.xml"
ren "xml\selection\classhise_1_1_doxygen_string_functions.xml" "String.xml"
ren "xml\selection\classhise_1_1_doxygen_array_functions.xml" "Array.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_f_f_t.xml" "FFT.xml"

ren "xml\selection\structhise_1_1_scripting_objects_1_1_script_download_object.xml" "Download.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_file.xml" "File.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_file_system.xml" "FileSystem.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_server.xml" "Server.xml"



ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripting_modulator.xml" "Modulator.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_midi_list.xml" "MidiList.xml"

ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_unordered_stack.xml" "UnorderedStack.xml"

ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_slider_pack_data.xml" "SliderPackData.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_table_data.xml" "Table.xml"


ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_audio_file.xml" "AudioFile.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripting_sampler_sound.xml" "Sample.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripting_message_holder.xml" "MessageHolder.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripting_effect.xml" "Effect.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripting_midi_processor.xml" "MidiProcessor.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripting_audio_sample_processor.xml" "AudioSampleProcessor.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripting_table_processor.xml" "TableProcessor.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_slider_pack_processor.xml" "SliderPackProcessor.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_display_buffer_source.xml" "DisplayBufferSource.xml"
ren "xml\selection\structhise_1_1_scripting_objects_1_1_script_modulation_matrix.xml" "ScriptModulationMatrix.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripted_look_and_feel.xml" "ScriptLookAndFeel.xml"
ren "xml\selection\classhise_1_1_script_user_preset_handler.xml" "UserPresetHandler.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripting_synth.xml" "ChildSynth.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripting_slot_f_x.xml" "SlotFX.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_routing_matrix.xml" "RoutingMatrix.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripting_preset_storage.xml" "PresetStorage.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_graphics_object.xml" "Graphics.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_path_object.xml" "Path.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_timer_object.xml" "Timer.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripted_midi_player.xml" "MidiPlayer.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripted_midi_automation_handler.xml" "MidiAutomationHandler.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_scripted_macro_handler.xml" "MacroHandler.xml"

ren "xml\selection\classhise_1_1_scripting_objects_1_1_markdown_object.xml" "MarkdownRenderer.xml"

ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_slider_1_1_modifier_object.xml" "Modifiers.xml"

ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_button.xml" "ScriptButton.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_combo_box.xml" "ScriptComboBox.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_slider.xml" "ScriptSlider.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_label.xml" "ScriptLabel.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_modulator_meter.xml" "ModulatorMeter.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_table.xml" "ScriptTable.xml"
ren "xml\selection\structhise_1_1_scripting_objects_1_1_script_background_task.xml" "BackgroundTask.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_thread_safe_storage.xml" "ThreadSafeStorage.xml"

ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_shader.xml" "ScriptShader.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_image.xml" "ScriptImage.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_panel.xml" "ScriptPanel.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_scripted_plotter.xml" "ScriptedPlotter.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_slider_pack.xml" "ScriptSliderPack.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_scripted_viewport.xml" "ScriptedViewport.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_floating_tile.xml" "ScriptFloatingTile.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_web_view.xml" "ScriptWebView.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_audio_waveform.xml" "ScriptAudioWaveform.xml"
ren "xml\selection\structhise_1_1_scripting_api_1_1_content_1_1_script_multipage_dialog.xml" "ScriptMultipageDialog.xml"

ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_buffer.xml" "Buffer.xml"
ren "xml\selection\structhise_1_1_scripting_objects_1_1_script_error_handler.xml" "ErrorHandler.xml"
ren "xml\selection\structhise_1_1_scripting_objects_1_1_script_broadcaster.xml" "Broadcaster.xml"

ren "xml\selection\classhise_1_1_script_loris_manager.xml" "LorisManager.xml"

ren "xml\selection\classhise_1_1_script_expansion_handler.xml" "ExpansionHandler.xml"
ren "xml\selection\classhise_1_1_script_expansion_reference.xml" "Expansion.xml"

ren "xml\selection\classscriptnode_1_1_dsp_network.xml" "DspNetwork.xml"
ren "xml\selection\structscriptnode_1_1_script_network_test.xml" "NetworkTest.xml"
ren "xml\selection\classscriptnode_1_1_node_base.xml" "Node.xml"
ren "xml\selection\classscriptnode_1_1_connection_base.xml" "Connection.xml"
ren "xml\selection\classscriptnode_1_1_node_base_1_1_parameter.xml" "Parameter.xml"

ren "xml\selection\structhise_1_1_scripting_objects_1_1_global_cable_reference.xml" "GlobalCable.xml"
ren "xml\selection\structhise_1_1_scripting_objects_1_1_global_routing_manager_reference.xml" "GlobalRoutingManager.xml"
ren "xml\selection\structhise_1_1_scripting_objects_1_1_script_builder.xml" "Builder.xml"

ren "xml\selection\structhise_1_1fixobj_1_1_array.xml" "FixObjectArray.xml"
ren "xml\selection\structhise_1_1fixobj_1_1_factory.xml" "FixObjectFactory.xml"
ren "xml\selection\structhise_1_1fixobj_1_1_stack.xml" "FixObjectStack.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_ring_buffer.xml" "DisplayBuffer.xml"

ren "xml\selection\structhise_1_1_script_unlocker_1_1_ref_object.xml" "Unlocker.xml"
ren "xml\selection\classhise_1_1_beatport_manager.xml" "BeatportManager.xml"
ren "xml\selection\classhise_1_1_scripting_api_1_1_date.xml" "Date.xml"
ren "xml\selection\classhise_1_1_scripting_objects_1_1_script_neural_network.xml" "NeuralNetwork.xml"

ApiExtractor.exe xml\selection xml\selection

del xml\selection\*.xml /Q

 BinaryBuilder.exe xml\selection "..\..\hi_scripting\scripting\api" XmlApi

del "xml\selection" /Q
REM #rd "xml\selection"

:END
