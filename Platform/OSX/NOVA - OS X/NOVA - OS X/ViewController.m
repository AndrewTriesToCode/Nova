//
//  ViewController.m
//  NOVA - OS X
//
//  Created by Andrew White on 1/28/16.
//  Copyright © 2016 Andrew White. All rights reserved.
//

#import "ViewController.h"

#include "../../../../Nova/nova_utility.h"
#include "../../../../Nova/nova_render.h"

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    NSString * path = [[NSBundle mainBundle] pathForResource:  @"f16" ofType: @"obj"];
    struct Mesh *mesh = CreateMeshFromFile((char *)path.UTF8String);

    [NSTimer scheduledTimerWithTimeInterval:0.01f target:self selector:@selector(handleTimer:) userInfo:nil repeats:YES];
}



- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}

- (void) handleTimer:(NSTimer *)timer {
    [self.view setNeedsDisplay:true];
}


@end
