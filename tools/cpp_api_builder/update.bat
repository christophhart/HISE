@echo off

set doxy_output_path=xml\xml\
set input_dir=input
set hise_input_dir=input\hise\
set raw_input_dir=input\raw\
set hise_prefix=%doxy_output_path%classhise_1_1
set hise_struct_prefix=%doxy_output_path%structhise_1_1
set raw_prefix=%hise_prefix%raw_1_1
set raw_struct_prefix=%hise_struct_prefix%raw_1_1
set output_dir=D:\Development\hise_documentation\cpp_api

echo C++ API Doc Builder script

echo Clear input directory
del /s /q %input_dir% 1>nul
rmdir /s /q %input_dir% 1>nul
mkdir %input_dir% 1>nul
del /s /q %doxy_output_path% 1>nul
rmdir /s /q %doxy_output_path% 1>nul
mkdir %doxy_output_path% 1>nul
del /s /q %output_dir% 1>nul
rmdir /s /q %output_dir% 1>nul
mkdir %output_dir% 1>nul

echo Building XML from doxygen...
doxygen
echo OK

echo Copying files from doxygen output folder

REM Copy Readme files
xcopy "%doxy_output_path%indexpage.xml" "%input_dir%" 1>nul
xcopy "%doxy_output_path%namespacehise.xml" "%hise_input_dir%" 1>nul
xcopy "%doxy_output_path%namespacehise_1_1raw.xml" "%raw_input_dir%" 1>nul

REM Copy HISE files
xcopy "%hise_prefix%_main_controller.xml" "%hise_input_dir%" 1>nul

xcopy "%hise_prefix%_modulator_chain.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_tempo_listener.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_processor.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_hise_event.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_midi_processor.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_modulator.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_modulator_synth.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_effect_processor.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_hise_event_buffer.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_project_handler.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_chain.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_audio_sample_processor.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_midi_player.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_controlled_object.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_event_id_handler.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_lockfree_queue.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_lookupt_table_processor.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_pool_base.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_pooled_u_i_updater.xml" "%hise_input_dir%" 1>nul
xcopy "%hise_prefix%_table.xml" "%hise_input_dir%" 1>nul

REM Copy hise struct files

xcopy "%hise_struct_prefix%_safe_function_call.xml" "%hise_input_dir%" 1>nul


REM Copy raw class files

xcopy "%raw_prefix%_builder.xml" "%raw_input_dir%" 1>nul
xcopy "%raw_prefix%_generic_storage.xml" "%raw_input_dir%" 1>nul
xcopy "%raw_prefix%_main_processor.xml" "%raw_input_dir%" 1>nul
xcopy "%raw_prefix%_data_holder_base.xml" "%raw_input_dir%" 1>nul
xcopy "%raw_prefix%_data_holder.xml" "%raw_input_dir%" 1>nul
xcopy "%raw_prefix%_plugin_parameter.xml" "%raw_input_dir%" 1>nul
xcopy "%raw_prefix%_pool.xml" "%raw_input_dir%" 1>nul
xcopy "%raw_prefix%_positioner.xml" "%raw_input_dir%" 1>nul
xcopy "%raw_prefix%_reference.xml" "%raw_input_dir%" 1>nul
xcopy "%raw_prefix%_storage.xml" "%raw_input_dir%" 1>nul
xcopy "%raw_prefix%_task_after_suspension.xml" "%raw_input_dir%" 1>nul
xcopy "%raw_prefix%_u_i_connection.xml" "%raw_input_dir%" 1>nul

REM Copy raw struct files

xcopy "%raw_struct_prefix%_data.xml" "%raw_input_dir%" 1>nul
xcopy "%raw_struct_prefix%_attribute.xml" "%raw_input_dir%" 1>nul


echo OK

echo Running Markdown converter

CppBuilder\Builds\VisualStudio2017\x64\Release\ConsoleApp\CppBuilder "%input_dir%" "%output_dir%"
echo OK

pause