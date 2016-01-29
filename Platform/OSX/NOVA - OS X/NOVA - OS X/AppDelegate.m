//
//  AppDelegate.m
//  NOVA - OS X
//
//  Created by Andrew White on 1/28/16.
//  Copyright © 2016 Andrew White. All rights reserved.
//

#import "AppDelegate.h"

#include "../../../../Nova/nova_utility.h"
#include "../../../../Nova/nova_render.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Insert code here to initialize your application
    NSString * path = [[NSBundle mainBundle] pathForResource:  @"f16" ofType: @"obj"];
    struct Mesh *mesh = CreateMeshFromFile(path.UTF8String);
    
    [NSTimer scheduledTimerWithTimeInterval:0.01f target:self selector:@selector(handleTimer:) userInfo:nil repeats:YES];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

- (void) handleTimer:(NSTimer *)timer {
    // Hanlde the timed event.
}

@end
