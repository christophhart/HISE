REM echo off

del "xml\selection" /Q
rd "xml\selection"

doxygen "xml.doxyfile"

md "xml\selection"

xcopy "xml\structsnex_1_1_types_1_1_frame_processor.xml" "xml\selection"
xcopy "xml\classhise_1_1_hise_event.xml" "xml\selection"
xcopy "xml\structsnex_1_1_types_1_1span.xml" "xml\selection"
xcopy "xml\structsnex_1_1_types_1_1dyn.xml" "xml\selection"



ren "xml\selection\structsnex_1_1_types_1_1_frame_processor.xml" "FrameProcessor.xml"
ren "xml\selection\classhise_1_1_hise_event.xml" "HiseEvent.xml"
ren "xml\selection\structsnex_1_1_types_1_1span.xml" "span.xml"
ren "xml\selection\structsnex_1_1_types_1_1dyn.xml" "dyn.xml"

CppBuilder.exe xml\selection output --valuetree



BinaryBuilder.exe output "..\..\hi_snex\api" SnexApi



:END
