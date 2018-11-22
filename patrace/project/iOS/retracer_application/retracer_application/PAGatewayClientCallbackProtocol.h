#pragma once

#import <Foundation/Foundation.h>

enum TraceStatus
{
    TraceStatus_EMPTY,
    TraceStatus_SCHEDULED,
    TraceStatus_RUNNING,
    TraceStatus_FINISHED,
    TraceStatus_FAILED
};

// Protocol allowing the gateway client to interact with the app.
@protocol PAGatewayClientCallbackProtocol <NSObject>

// Start and query state of retrace
-(void)             startRetrace:(NSString*)json storagePath:(NSString*)storagePath resultFile:(NSString*) resultFile;
-(enum TraceStatus) getRetraceStatus;

// Inform about gateway connection status (connected/disconnected, filetransfer, etc).
-(void) updateConnectionStatus:(bool)state progress:(int)progress;

@end

// Recives callbacks from the gateway client.
// Global because it is referenced by the gateway client plugins, which we can't
// easily inject a callback-ptr into.
// Defined in IOSPaRetracePlugin.mm
extern id <PAGatewayClientCallbackProtocol> gIOSGatewayCallback;