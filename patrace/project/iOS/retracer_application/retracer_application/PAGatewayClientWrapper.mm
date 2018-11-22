#import "PAGatewayClientWrapper.h"

#include "IOSDeviceInterface.h"
#include "gateway_client/Client.h"
#include <assert.h>

@interface PAGatewayClientWrapper ()
{
    Client* _client;
    IOSDeviceInterface* _deviceInterface;
    bool _threadRunning;
}
@property (atomic) NSThread* gatewayClientThread; // Thread to run the gateway client in
@end

@implementation PAGatewayClientWrapper

- (id) init
{
    self = [super init];
    
    // Create -- but don't start -- thread in which the gateway client
    // will run.
    self.gatewayClientThread = [[NSThread alloc] initWithTarget:self selector:@selector(gatewayClientThreadMethod) object:nil];
    
    // iOS-spesific subclass of DeviceInterface
    _deviceInterface = new IOSDeviceInterface;
    
    // Create Client (the gateway _client_).
    // It takes ownership of the device-interface, so the interface does not have to be deleted.
    _client = new Client(_deviceInterface, Json::Value());
    
    return self;
}

-(void) dealloc
{
    delete _client;
}

- (void) start:(id<PAGatewayClientCallbackProtocol>)callback gatewayURL:(NSString *)gwURL
{
    // Set up callback used by the gateway client
    assert(gIOSGatewayCallback == nil);
    gIOSGatewayCallback = callback;
    
    // Set up gateway client with gatewau URL
    _client->setBaseUrl([gwURL UTF8String]);
    
    // Start thread which runs the gateway client
    assert(![self.gatewayClientThread isExecuting]);
    [self.gatewayClientThread start];
}

- (void) setGatewayURL:(NSString*) url
{
    _client->setBaseUrl([url UTF8String]);
}

- (void) setExitAfterTask: (bool) yesno
{
    _client->setExitAfterTask(yesno);
}

-(void) gatewayClientThreadMethod
{
    _client->run();
}

- (void) stop
{
    // This is a bit weird, but instead of stopping the client directly,
    // we set state in the DeviceInterface which will be picked up on
    // by the gateway client and cause it to stop.
    _deviceInterface->setShouldBeRunning(false);
    
    // Block until thread has stopped.
    // This blocks the main thread (UI), but
    // is simpler than setting up a callback-timer.
    // Note that depending on what the gateway client is doing,
    // this might actually take quite a long time (multiple seconds).
    while([self.gatewayClientThread isExecuting])
    {
        usleep(1000);
    }
    
    gIOSGatewayCallback = nil;
}

@end
