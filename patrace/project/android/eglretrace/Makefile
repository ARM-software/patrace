# Generate retrace code and C header

default_target: callNdkBuild
.PHONY : default_target

autoGenRetrace:
	@echo 'Remove the existing auto-generated source files...'
	rm -f ../../../src/common/api_info_auto.cpp
	rm -f ../../../src/dispatch/eglproc_auto.hpp
	rm -f ../../../src/dispatch/eglproc_auto.cpp
	rm -f ../../../src/retracer/retrace_gles_auto.cpp
	rm -f ../../../src/specs/khronos_enums.hpp
	@echo 'Auto-generate the retrace code...'
	cd ../../../src/common && python api_info.py
	cd ../../../src/common && python call_parser.py
	cd ../../../src/dispatch && python eglproc.py
	cd ../../../src/retracer && python retrace.py
	cd ../../../src/specs/ && python glxml_header.py
.PHONY : autoGenRetrace


callNdkBuild:autoGenRetrace
	@echo 'Generate C header for NativeAPI.java'
	ANDROID_JAR=$(shell dirname `which android`)/../platforms/android-${TARGET}/android.jar; \
	    javac -classpath $$ANDROID_JAR ./src/com/arm/pa/paretrace/NativeAPI.java; \
	    javah -d jni -classpath $$ANDROID_JAR:src com.arm.pa.paretrace.NativeAPI
.PHONY : callNdkBuild