#import "PAMainMenuViewController.h"
#import "PATraceFilesViewController.h"
#import "PAGatewayClientViewController.h"

@interface PAMainMenuViewController ()

@end

@implementation PAMainMenuViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Exit-button
        self.navigationItem.hidesBackButton = YES;
        UIBarButtonItem *customButton = [[UIBarButtonItem alloc] initWithTitle:@"exit(0)" style:UIBarButtonItemStylePlain target:self action:@selector(exitButtonPressed:)];
        self.navigationItem.leftBarButtonItem = customButton;
    }
    return self;
}

-(void)exitButtonPressed:(id)sender
{
    exit(0);
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    self.navigationItem.title = @"Main menu";
    
    [self launchGatewayClient:self];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

-(IBAction)launchGatewayClient:(id)sender
{
    PAGatewayClientViewController* gatewayView = [[PAGatewayClientViewController alloc] init];
    [self.navigationController pushViewController:gatewayView animated:YES];
}

-(IBAction)launchTraceView:(id)sender
{
    PATraceFilesViewController* traceSelectView = [[PATraceFilesViewController alloc] init];
    [self.navigationController pushViewController:traceSelectView animated:YES];
}

@end
