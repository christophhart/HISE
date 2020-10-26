REM echo off

del "xml\selection" /Q
rd "xml\selection"

doxygen "xml.doxyfile"

md "xml\selection"

xcopy "xml\structsnex_1_1_types_1_1_frame_processor.xml" "xml\selection"
xcopy "xml\classhise_1_1_hise_event.xml" "xml\selection"
xcopy "xml\structsnex_1_1_types_1_1span.xml" "xml\selection"
xcopy "xml\structsnex_1_1_types_1_1dyn.xml" "xml\selection"
xcopy "xml\structsnex_1_1_types_1_1sfloat.xml" "xml\selection"
xcopy "xml\structsnex_1_1_types_1_1_prepare_specs.xml" "xml\selection"
xcopy "xml\structsnex_1_1_types_1_1_osc_process_data.xml" "xml\selection"
xcopy "xml\structsnex_1_1_types_1_1_process_data.xml" "xml\selection"
xcopy "xml\structscriptnode_1_1container_1_1chain.xml" "xml\selection"

structscriptnode_1_1container_1_1chain

ren "xml\selection\structsnex_1_1_types_1_1_frame_processor.xml" "FrameProcessor.xml"
ren "xml\selection\classhise_1_1_hise_event.xml" "HiseEvent.xml"
ren "xml\selection\structsnex_1_1_types_1_1span.xml" "span.xml"
ren "xml\selection\structsnex_1_1_types_1_1dyn.xml" "dyn.xml"
ren "xml\selection\structsnex_1_1_types_1_1sfloat.xml" "sfloat.xml"
ren "xml\selection\structsnex_1_1_types_1_1_prepare_specs.xml" "PrepareSpecs.xml"
ren "xml\selection\structsnex_1_1_types_1_1_osc_process_data.xml" "OscProcessData.xml"
ren "xml\selection\structsnex_1_1_types_1_1_process_data.xml" "ProcessData.xml"
ren "xml\selection\structscriptnode_1_1container_1_1chain.xml" "container_chain.xml"

CppBuilder.exe xml\selection output --valuetree



BinaryBuilder.exe output "..\..\hi_snex\api" SnexApi



:END
