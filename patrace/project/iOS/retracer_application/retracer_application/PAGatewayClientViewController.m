#import "PAGatewayClientViewController.h"
#import "PARetraceViewController.h"
#import "PAGatewayClientWrapper.h"

@interface PAGatewayClientViewController ()

// Wrapper for the gateway client
@property (atomic) PAGatewayClientWrapper* gatewayClientWrapper;

// Indicates file transfers and connction status
@property (nonatomic) UIProgressView* progressView;
@property (nonatomic) UIActivityIndicatorView* connectionStatusIndicator;
@property (nonatomic) UILabel* gatewayUrlLabel;
@property (nonatomic) UILabel* traceResultMessageLabel;

// Switch to toggle automatic restart after each task (actually exit(0), but causes restart when in Guided Access mode)
@property (nonatomic) UISwitch* restartAfterTaskSwitch;
@property (nonatomic) UILabel* restartAfterTaskLabel;

// Track retracing state
@property (atomic) enum TraceStatus traceStatus;

@end

@implementation PAGatewayClientViewController

NSString * const kExitSwitchStr = @"restartAfterTask";

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    
    if (self)
    {       
        self.traceStatus = TraceStatus_EMPTY;
        
        self.gatewayClientWrapper = [[PAGatewayClientWrapper alloc] init];
        
        NSString* gwURL = [self getGatewayURL];
        [self startGatewayClient:gwURL];
        
        // TODO Display URL in UI
        
        // Custom back button which stops the gateway client thread before exiting the view
        self.navigationItem.hidesBackButton = YES;
        UIBarButtonItem *customButton = [[UIBarButtonItem alloc] initWithTitle:@"Back" style:UIBarButtonItemStylePlain target:self action:@selector(backButtonPressed:)];
        self.navigationItem.leftBarButtonItem = customButton;
        
        // Ask user for new gateway URL
        UIBarButtonItem *editURLButton = [[UIBarButtonItem alloc] initWithTitle:@"Change URL" style:UIBarButtonItemStylePlain target:self action:@selector(changeURLButtonPressed:)];
        self.navigationItem.rightBarButtonItem = editURLButton;
    }
    return self;
}

- (NSString*) getGatewayURL
{
    // Get stored gateway URL
    NSString* gatewayURL = [[NSUserDefaults standardUserDefaults] stringForKey:@"gatewayURL"];
    if ([gatewayURL length] < 1)
    {
        gatewayURL = @"http://trd-patgw:10002";
    }
    return gatewayURL;
}

- (void) changeURLButtonPressed:(id) sender
{
    // Show alert asking for gateway URL, using the stored URL as the preset default.
    UIAlertView* alert = [[UIAlertView alloc] initWithTitle:@"Gateway URL" message:@"Input gateway URL" delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
    alert.alertViewStyle = UIAlertViewStylePlainTextInput;

    NSString* gwUrl = [self getGatewayURL];
    [alert textFieldAtIndex:0].text = gwUrl;

    [alert show];
}

// Called when user confirms gateway URL alert view
-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    // Get URL from alertview
    NSString* gwUrl = [alertView textFieldAtIndex:0].text;
    
    // Store as default
    [[NSUserDefaults standardUserDefaults] setObject:gwUrl forKey:@"gatewayURL"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    // Update label
    self.gatewayUrlLabel.text = gwUrl;
    
    // Update gateway client
    [self.gatewayClientWrapper setGatewayURL:gwUrl];
}

- (void) startGatewayClient:(NSString*) gatewayURL
{
    // Start gateway client (through wrapper)
    [self.gatewayClientWrapper start:self gatewayURL:gatewayURL];
}

-(void)backButtonPressed:(id)sender
{
    // Ask client to stop. (This can take a while.)
    [self.gatewayClientWrapper stop];
    
    [self.navigationController popViewControllerAnimated:YES];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.navigationItem.title = @"Gateway Client";
    
    int startY = 65;
    
    self.gatewayUrlLabel = [[UILabel alloc] initWithFrame:CGRectMake(10, startY, 250, 50.0)];
    self.gatewayUrlLabel.numberOfLines = 0;
    self.gatewayUrlLabel.textAlignment = NSTextAlignmentCenter;
    self.gatewayUrlLabel.text = [self getGatewayURL];
    [self.view addSubview:self.gatewayUrlLabel];
    
    startY += 45;
    
    self.connectionStatusIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
    self.connectionStatusIndicator.frame = CGRectMake(10, startY, 250, 25);
    [self.connectionStatusIndicator startAnimating];
    [self.view addSubview:self.connectionStatusIndicator];
    
    startY += 35;
    
    self.progressView = [[UIProgressView alloc] initWithProgressViewStyle:UIProgressViewStyleDefault];
    self.progressView.frame = CGRectMake(10, startY, 250, 10);
    [self.view addSubview:self.progressView];
    
    startY += 10;
    
    self.restartAfterTaskSwitch = [[UISwitch alloc] initWithFrame:CGRectMake(10, startY, 250, 50)];
    [self.restartAfterTaskSwitch addTarget:self action:@selector(toggleExitAfterTask:) forControlEvents:UIControlEventValueChanged];
    self.restartAfterTaskSwitch.on = NO;
    [self.view addSubview:self.restartAfterTaskSwitch];
    
    // Load stored default if it exists
    if ([[NSUserDefaults standardUserDefaults] objectForKey:kExitSwitchStr])
    {
        bool restartAfterTask = [[NSUserDefaults standardUserDefaults] boolForKey:kExitSwitchStr];
        self.restartAfterTaskSwitch.on = restartAfterTask;
    }
    
    // Apply setting
    [self.gatewayClientWrapper setExitAfterTask:self.restartAfterTaskSwitch.on];
    
    self.restartAfterTaskLabel = [[UILabel alloc] initWithFrame:CGRectMake(75, startY-7.5, 250, 50.0)];
    self.restartAfterTaskLabel.textAlignment = NSTextAlignmentLeft;
    self.restartAfterTaskLabel.text = @"Exit after task";
    [self.view addSubview:self.restartAfterTaskLabel];
    
    startY += 20;
    
    self.traceResultMessageLabel = [[UILabel alloc] initWithFrame:CGRectMake(10, startY, 250, 50.0)];
    self.traceResultMessageLabel.numberOfLines = 0;
    self.traceResultMessageLabel.textAlignment = NSTextAlignmentCenter;
    self.traceResultMessageLabel.text = @"";
    [self.view addSubview:self.traceResultMessageLabel];
    
    startY += 20;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    // If we're visible, either we started for the first time or trace finished
    assert(self.traceStatus == TraceStatus_RUNNING || self.traceStatus == TraceStatus_EMPTY);
    
    if(self.traceStatus == TraceStatus_RUNNING)
        self.traceStatus = TraceStatus_FINISHED;
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
}

- (void) toggleExitAfterTask:(UISwitch*) sw
{
    [self.gatewayClientWrapper setExitAfterTask:self.restartAfterTaskSwitch.on];
    
    // Save setting
    [[NSUserDefaults standardUserDefaults] setBool:self.restartAfterTaskSwitch.on forKey:kExitSwitchStr];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

#pragma mark PAGatewayClientCallbackProtocol implementation
// The below methods are called from the gateway client thread!

-(enum TraceStatus) getRetraceStatus
{
    return self.traceStatus;
}

-(void) startRetrace:(NSString*)json storagePath:(NSString*)storagePath resultFile:(NSString*) resultFile
{
    assert(self.traceStatus == TraceStatus_EMPTY||self.traceStatus == TraceStatus_FINISHED);
    self.traceStatus = TraceStatus_SCHEDULED;
    
    // Run on main thread
    dispatch_async(dispatch_get_main_queue(),
    ^{
        // Create and start retrace-view
        PARetraceViewController* retraceView = [[PARetraceViewController alloc] initWithJSON:[json UTF8String] storagePath:[storagePath UTF8String] resultFile:[resultFile UTF8String]];
        
        // Clear message, and pass it to the new view
        self.traceResultMessageLabel.text = @"";
        retraceView.resultMessageLabel = self.traceResultMessageLabel;
    
        assert(_traceStatus == TraceStatus_SCHEDULED);
        self.traceStatus = TraceStatus_RUNNING;
    
        [self.navigationController pushViewController:retraceView animated:YES];
        
    });
}

-(void) updateConnectionStatus:(bool)state progress:(int)progress
{
    dispatch_async(dispatch_get_main_queue(),
    ^{
        // Update indicator to show connection status
        if(state ==0)
        {
            self.connectionStatusIndicator.color = [UIColor redColor];
        }
        else
        {
            self.connectionStatusIndicator.color = [UIColor greenColor];
        }
        
        // Update progress-bar to show progress
        if(progress == -1)
        {
            [self.progressView setProgress:1.0 animated:YES];
        }
        else
        {
            [self.progressView setProgress:progress/100.0f animated:YES];
        }
    });
}

@end


