#pragma once

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@interface PARetraceViewController : GLKViewController
-(id) initWithTraceFilePath:(NSString*) filePath;
-(id) initWithJSON:(const char *)jsonData storagePath:(const char *)storagePath resultFile:(const char *)resultFilePath;
@property UILabel* resultMessageLabel;
@end
