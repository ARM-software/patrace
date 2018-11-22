#pragma once

#include "gateway_client/DeviceInterface.h"

class IOSDeviceInterface : public DeviceInterface
{
public:
    IOSDeviceInterface();
    virtual ~IOSDeviceInterface();
    
    void setShouldBeRunning(bool running);
    
private:
    bool mShouldBeRunning;
    virtual void doUpdateConnectionStatus(int state, int progress) const;
    virtual bool doIsClientRunning() const;
    virtual void doGetDeviceInfo() const;
    
    // Not supported
    bool initGPUSysFSConfig() const { return false; }
    bool setDVFS(bool on) { return false; }
    bool setGPUFrequency(unsigned int freq) { return false; }
};