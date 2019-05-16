# Please specify NDK.

default_target: callNdkBuild
.PHONY : default_target

autoGenCode:
	@echo 'Remove the existing auto-generated source files...'
	rm -f ../../../src/fakedriver/egl/auto.cpp
	rm -f ../../../src/fakedriver/gles1/auto.cpp
	rm -f ../../../src/fakedriver/gles2/auto.cpp
	rm -f ../../../src/fakedriver/single/auto.cpp
	@echo 'Auto-generate the source code for fake driver...'
	cd ../../../src/fakedriver && python autogencode.py
.PHONY : autoGenCode

callNdkBuild: autoGenCode
	@echo 'Build the driver interfaces with Android-NDK...'
	+$(NDK)/ndk-build -B
.PHONY : callNdkBuild

