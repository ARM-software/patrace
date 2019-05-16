default_target: callNdkBuild
.PHONY : default_target

autoGenTrace:
	@echo 'Remove the existing auto-generated source files...'
	rm -f ../../../../../src/specs/pa_gl31.h ../../../../../src/specs/pa_gl31.cpp
	@echo 'Auto-generate the retrace code...'
	cd ../../../../../src/specs/ && python glxml_to_funcs.py
.PHONY : autoGenTrace

callNdkBuild:autoGenTrace
	+$(NDK)/ndk-build clean
	+$(NDK)/ndk-build NDK_DEBUG=1
	ant clean
	ant release
.PHONY : callNdkBuild
