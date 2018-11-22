#pragma once

#import <UIKit/UIKit.h>
#import "PAGatewayClientCallbackProtocol.h"

// Runs the gateway client
@interface PAGatewayClientViewController : UIViewController <PAGatewayClientCallbackProtocol,UIAlertViewDelegate>
@end
