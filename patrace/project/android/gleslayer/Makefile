# Generate code for gleslayer

autoGenTrace:
	@echo 'Remove the existing auto-generated source files...'
	#rm -f ../../../src/common/api_info_auto.cpp
	#rm -f ../../../src/dispatch/eglproc.hpp
	#rm -f ../../../src/helper/paramsize.cpp
	#rm -f ../../../src/tracer/sig_enum.hpp
	#rm -f ../../../src/tracer/egltrace_auto.cpp
	#rm -f ../../../src/specs/khronos_enums.hpp
	@echo 'Auto-generate the source code for gleslayer...'
	#cd ../../../src/common && python api_info.py
	#cd ../../../src/dispatch && python eglproc.py
	#cd ../../../src/helper && python paramsize.py
	#cd ../../../src/tracer && python trace.py
	#cd ../../../src/specs/ && python glxml_header.py
.PHONY : autoGenTrace
