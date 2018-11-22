#pragma once

#import <Foundation/Foundation.h>
#import "PAGatewayClientCallbackProtocol.h"

@interface PAGatewayClientWrapper : NSObject

- (id) init;

// Starts the gateway client in a separate thread, and sets it up
// to communicate using the passed callback
- (void) start:(id<PAGatewayClientCallbackProtocol>) callback gatewayURL:(NSString *)gwURL;

// Stops the gateway client thread. Can take a while (seconds).
- (void) stop;

- (void) setGatewayURL:(NSString*) url;
- (void) setExitAfterTask: (bool) yesno;
@end
