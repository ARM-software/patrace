LOCAL_PATH:=$(call my-dir)/../../../../../.. # to patrace dir

##############################################################
# Target: integration test

include $(CLEAR_VARS)

LOCAL_MODULE := TEMPLATE_APP_NAME

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/src/integration_tests \
	$(LOCAL_PATH)/src/specs \
	$(LOCAL_PATH)/../thirdparty/opengl-registry/api \
LOCAL_SRC_FILES := src/integration_tests/TEMPLATE_APP_NAME.cpp src/integration_tests/pa_demo.cpp src/specs/pa_gl31.cpp
LOCAL_LDLIBS := -landroid -llog -lEGL -lGLESv3 -lstdc++
LOCAL_CFLAGS := -frtti -D__arm__ -D__gnu_linux__ -Wall -std=c++11
LOCAL_WHOLE_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/native_app_glue)
