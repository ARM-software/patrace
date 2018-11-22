#import "PARetraceViewController.h"
#import "iOS_API.h"
#include "common/pa_exception.h"

@interface PARetraceViewController ()
{
    SelectedConfig selectedConfig; // This is partially filled out in viewDidLoad and finished in drawInRect
    GLuint doneTracing; // 0 when done (view is popped), 1 while running
}
@property (nonatomic) EAGLContext* context;
@property (nonatomic) bool hasAppeared; // Can only pop when done once successfully appeared; this tracks this
@property (nonatomic) bool firstFrame; // Done once at retrace start
@property (nonatomic) NSInteger orientationMask;
@end

@implementation PARetraceViewController

-(id) init
{
    self = [super init];
    self.firstFrame = true;
    self.hasAppeared = false;
    self.resultMessageLabel = nil;
    self.orientationMask = UIInterfaceOrientationMaskLandscape;
    doneTracing = 1;
    return self;
}

// Called when manually selecting a tracefile to run
-(id)initWithTraceFilePath:(NSString *)filePath
{
    self = [self init];
    
    GLKView *view = (GLKView *)self.view;
    
    //iOSRetracer_init([view drawableWidth], [view drawableHeight]);
    //iOSRetrace_setForceOffscreen(false);
    //iOSRetrace_setBeginMeasureFrame(2);
    
    if([self startTraceOfFile:filePath] == 0) {
        doneTracing = 0;
    }
    
    return self;
}

// Called by the gateway client
-(id)initWithJSON:(const char *)jsonData storagePath:(const char *)storagePath resultFile:(const char *)resultFilePath
{
    self = [self init];
    
    if(iOSRetracer_initWithJSON(jsonData, storagePath, resultFilePath) == 0)
        doneTracing = 0;
    
    return self;
}

-(BOOL)shouldAutorotate
{
    return YES;
}

-(NSUInteger)supportedInterfaceOrientations
{
    return self.orientationMask;
}

-(BOOL)prefersStatusBarHidden
{
    return YES;
}

-(void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    self.hasAppeared = true;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    // Set up button to abort trace
    UIBarButtonItem *abortButton = [[UIBarButtonItem alloc] initWithTitle:@"Abort" style:UIBarButtonItemStylePlain target:self action:@selector(abortButtonPressed:)];
    self.navigationItem.leftBarButtonItem = abortButton;
    
    // Uncomment to enable glGetError()s etc.
    //iOSRetracer_setDebug(true);
    
    retracer::RetraceOptions* retraceOptions = iOSRetracer_getRetracerOptions();
    
    // Create context
    if(retraceOptions->mApiVersion == retracer::PROFILE_ES1)
    {
        self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
    }
    else if (retraceOptions->mApiVersion == retracer::PROFILE_ES2)
    {
        self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    }
    else
    {
        self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    }
    
    if (!self.context)
    {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    
    view.context = self.context;
    [EAGLContext setCurrentContext:view.context];
    
    int r, g, b, a, depth, stencil, msaa;
    r = g = b = a = depth = stencil = msaa = 0;

    // Set up state from RetraceOptions
    if (retraceOptions->mOnscreenConfig.red   > 5 ||
        retraceOptions->mOnscreenConfig.green > 6 ||
        retraceOptions->mOnscreenConfig.blue  > 5 ||
        retraceOptions->mOnscreenConfig.alpha > 0)
    {
        view.drawableColorFormat = GLKViewDrawableColorFormatRGBA8888;
        r = g = b = a = 8;
        NSLog(@"Using RGBA8888\n");
    }
    else
    {
        view.drawableColorFormat = GLKViewDrawableColorFormatRGB565;
        r = 5;
        g = 6;
        b = 5;
        a = 0;
        NSLog(@"Using RGB565\n");
    }
    
    // Select depth bits
    if (retraceOptions->mOnscreenConfig.depth > 16)
    {
        view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
        depth = 24;
        NSLog(@"Using Depth24\n");
    }
    else if (retraceOptions->mOnscreenConfig.depth > 0)
    {
        view.drawableDepthFormat =  GLKViewDrawableDepthFormat16;
        depth = 16;
        NSLog(@"Using Depth16\n");
    }
    else
    {
        view.drawableDepthFormat = GLKViewDrawableDepthFormatNone;
        depth = 0;
        NSLog(@"Using DepthNone\n");
    }
    
    // Select stencil bits
    if (retraceOptions->mOnscreenConfig.stencil > 0)
    {
        view.drawableStencilFormat = GLKViewDrawableStencilFormat8;
        stencil = 8;
        NSLog(@"Using Stencil8\n");
    }
    else
    {
        view.drawableStencilFormat = GLKViewDrawableStencilFormatNone;
        stencil = 0;
        NSLog(@"Using StencilNone\n");
    }
    
    if (retraceOptions->mOnscreenConfig.msaa_samples > 0)
    {
        view.drawableMultisample = GLKViewDrawableMultisample4X;
        msaa = 4;
        NSLog(@"Using 4x MSAA");
    }
    else
    {
        view.drawableMultisample = GLKViewDrawableMultisampleNone;
        msaa = 0;
        NSLog(@"Using no MSAA");
    }
    
    // Remember what should be the selected config
    selectedConfig.eglConfig = EglConfigInfo(r, g, b, a, depth, stencil, msaa, 0);
    
    // Make sure it tries to draw as fast as possible
    self.preferredFramesPerSecond = 100;
    
    // Force orientation if not offscreen
    if(!retraceOptions->mForceOffscreen)
    {
        if(retraceOptions->mWindowHeight > retraceOptions->mWindowWidth)
        {
            [[UIDevice currentDevice] setValue:[NSNumber numberWithInteger:UIInterfaceOrientationPortrait] forKeyPath:@"orientation"];
            self.orientationMask = UIInterfaceOrientationMaskPortrait;
        }
        else
        {
            [[UIDevice currentDevice] setValue:[NSNumber numberWithInteger:UIInterfaceOrientationLandscapeLeft] forKeyPath:@"orientation"];
            self.orientationMask = UIInterfaceOrientationMaskLandscape;
        }
    }
    
    // Hide navigation bar
    [self.navigationController setNavigationBarHidden:YES];
}

- (void)dealloc
{
    if ([EAGLContext currentContext] == self.context)
    {
        [EAGLContext setCurrentContext:nil];
    }
}

-(void)abortButtonPressed:(id)sender
{
    doneTracing = 1;
    
    // This is called automactially after a successfull trace,
    // but not if we abort.
    iOSRetracer_closeTraceFile();
}

// Called on manual tracefile select
- (int)startTraceOfFile: (NSString*)fullPath
{
    return iOSRetracer_retraceFile([fullPath UTF8String]);
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    
    // iOS is very conservative when it comes to memory warnings,
    // meaning a large number of traces will be unneccesarily aborted
    // if we heed it. Commenting out the return will
    // abort the retrace on warnings.
    return;
    
    // Handle this as an abort
    [self abortButtonPressed:self];
    self.resultMessageLabel.text = @"Aborted: received memory warning.";
    
    if ([self isViewLoaded] && ([[self view] window] == nil))
    {
        self.view = nil;
        
        if ([EAGLContext currentContext] == self.context)
        {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    if (self.firstFrame)
    {
        self.firstFrame = NO;
        
        // Finish filling out selectedConfig
        selectedConfig.surfaceWidth = (int) [view drawableWidth];
        selectedConfig.surfaceHeight = (int) [view drawableHeight];
        
        if(iOSRetracer_setAndCheckSelectedConfig(selectedConfig) != 0)
        {
            // Not valid config for trace
            if(self.resultMessageLabel)
                self.resultMessageLabel.text = @"The selected configuration was not valid";
            
            [self abortButtonPressed:self];
            return;
        }
    }
    
    if (!doneTracing)
    {
        try
        {
            iOSRetracer_TraceState state = iOSRetracer_retraceUntilSwapBuffers();
            
            // Check if done
            if (state != iOSRetracer_TRACE_NOT_FINISHED)
            {
                doneTracing = 1; // Trace done
               
                // Set result according to why trace stopped
                if(self.resultMessageLabel)
                {
                    self.resultMessageLabel.text = @"Trace stopped for unknown reason.";
                    
                    if(state == iOSRetracer_TRACE_FAILED_OUT_OF_MEMORY)
                        self.resultMessageLabel.text = @"Trace failed: out of memory.";
                    if(state == iOSRetracer_TRACE_FAILED_FAILED_TO_LINK_SHADER_PROGRAM)
                        self.resultMessageLabel.text = @"Trace failed: failed to link shader program.";
                    else if(state == iOSRetracer_TRACE_FINISHED_OK)
                        self.resultMessageLabel.text = @"Trace finished successfully.";
                }
            }
        }
        catch (const PAException& error)
        {
            // Trace have failed. Abort retrace.
            NSLog(@"%s", error.what());
            
            if(self.resultMessageLabel)
                self.resultMessageLabel.text = [NSString stringWithUTF8String:error.what()];
            
            [self abortButtonPressed:self];
        }
        catch (...)
        {
            // Trace have failed. Abort retrace.
            NSLog(@"Got unknown exception.");
            
            if(self.resultMessageLabel)
                self.resultMessageLabel.text = @"Got unknown excepetion";
            
            [self abortButtonPressed:self];
        }
    }
    
    // Pop view when done
    if(doneTracing && self.hasAppeared)
    {
        [self.navigationController setNavigationBarHidden:NO];
        [self.navigationController popViewControllerAnimated:YES];
    }
}

@end
