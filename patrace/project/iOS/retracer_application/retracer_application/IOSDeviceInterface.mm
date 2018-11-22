#include "IOSDeviceInterface.h"

#import "PAGatewayClientCallbackProtocol.h"
#import "ios_rev.h"

#import <UIKit/UIScreen.h>
#import <UIKit/UIDevice.h>
#import <OpenGLES/EAGL.h>

#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/machine.h>

#include "common/os.hpp"

IOSDeviceInterface::IOSDeviceInterface()
    : DeviceInterface(DeviceInfo()), mShouldBeRunning(true)

{

}

IOSDeviceInterface::~IOSDeviceInterface()
{
    
}

void IOSDeviceInterface::doUpdateConnectionStatus(int state, int progress) const
{
    DBG_LOG("Connected: %d, File transfer: %d%%\n", state, progress);
    [gIOSGatewayCallback updateConnectionStatus:state progress:progress];
}

void IOSDeviceInterface::setShouldBeRunning(bool running)
{
    mShouldBeRunning = running;
}

bool IOSDeviceInterface::doIsClientRunning() const
{
    return mShouldBeRunning;
}

void IOSDeviceInterface::doGetDeviceInfo() const
{
    mDeviceInfo.device_name = [[[UIDevice currentDevice] name] UTF8String];
    //mDeviceInfo.device_model = [[[UIDevice currentDevice] model] UTF8String];
    
    struct utsname systemInfo;
    uname(&systemInfo);
    mDeviceInfo.device_model = systemInfo.machine;

    mDeviceInfo.os_name = [[[UIDevice currentDevice] systemName] UTF8String];
    mDeviceInfo.os_version = [[[UIDevice currentDevice] systemVersion] UTF8String];
    
    EAGLContext* c = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    
    if (c == nil) {
        c = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    }
    
    if(c && [EAGLContext setCurrentContext:c])
    {
        // Get extensions
        GLint numExtensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    
        for (int i = 0; i < numExtensions; i++)
        {
            mDeviceInfo.supported_extensions.push_back((GLchar*) glGetStringi(GL_EXTENSIONS, i));
        }
        
        // Get info
        mDeviceInfo.gl_renderer = (GLchar*) glGetString(GL_RENDERER);
        mDeviceInfo.gl_vendor = (GLchar*) glGetString(GL_VENDOR);
        mDeviceInfo.gl_version = (GLchar*) glGetString(GL_VERSION);
        mDeviceInfo.gl_shader_version = (GLchar*) glGetString(GL_SHADING_LANGUAGE_VERSION);
        
        // Get supported compressed texture formats
        GLint numTextureFormats;
        glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &numTextureFormats);
        
        GLint textureFormats[numTextureFormats];
        glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, textureFormats);
        
        for(int i = 0; i < numTextureFormats; ++i)
        {
            switch(textureFormats[i])
            {
                case  GL_COMPRESSED_RGB8_ETC2:
                {
                    mDeviceInfo.supported_textures.push_back("etc2");
                    mDeviceInfo.supported_textures.push_back("etc1");
                    // ETC2 is backwards compatible with ETC1, and we
                    // override GL_ETC1_RGB8_OES to GL_COMPRESSED_RGB8_ETC2 when parsing
                    // the internalformat if __APPLE__ is defined (see call_parser.py)
                    break;
                }
            }
        }
        
        [EAGLContext setCurrentContext:nil];
    }
    
    // Always support for noncompressed textures
    mDeviceInfo.supported_textures.push_back("nocomp");
    
    // Architecture
    NSMutableString *cpu = [[NSMutableString alloc] init];
    size_t size;
    cpu_type_t type;
    cpu_subtype_t subtype;
    size = sizeof(type);
    sysctlbyname("hw.cputype", &type, &size, NULL, 0);
    
    size = sizeof(subtype);
    sysctlbyname("hw.cpusubtype", &subtype, &size, NULL, 0);
    
    if (type & (~CPU_ARCH_MASK) & CPU_TYPE_ARM)
    {
        [cpu appendString:@"ARM"];
        if(type & CPU_ARCH_ABI64)
           [cpu appendString:@"64"];
    } else {
        [cpu appendString:@"Not recognized"];
    }
    
    mDeviceInfo.os_arch = [cpu UTF8String];
    mDeviceInfo.os_build = "Unknown";
    mDeviceInfo.os_build_time = "0";
    
    // Screensize
    CGRect screenRect = [[UIScreen mainScreen] bounds];
    CGFloat screenScale = [[UIScreen mainScreen] scale];
    mDeviceInfo.resolution.push_back(screenRect.size.width * screenScale);
    mDeviceInfo.resolution.push_back(screenRect.size.height * screenScale);
    
    mDeviceInfo.board = "N/A";
    mDeviceInfo.abi = "?";
    
    // Version info
#ifdef IOS_REV
    mDeviceInfo.gw_client_version = IOS_REV;
#else
    mDeviceInfo.gw_client_version = "Unknown (IOS_REV not defined)";
#endif
}
