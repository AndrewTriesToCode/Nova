//
//  ViewController.m
//  NOVA - OS X
//
//  Created by Andrew White on 1/28/16.
//  Copyright © 2016 Andrew White. All rights reserved.
//

#import "ViewController.h"
#import "NovaView.h"
#import <QuartzCore/QuartzCore.h> // for timer


#include "../../../../Nova/nova_utility.h"
#include "../../../../Nova/nova_math.h"
#include "../../../../Nova/nova_render.h"

@interface ViewController()

@property (nonatomic) NovaView *novaView;
@property (nonatomic) CFTimeInterval current_time;

@end

struct Matrix rot, rot2, trans, pos, pos1, screen_mat, proj_mat;
struct RenderContext context;
struct Mesh *mesh;
float ang = 0.0f;

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    NSString * path = [[NSBundle mainBundle] pathForResource:  @"f16" ofType: @"obj"];
    
    self.novaView = (NovaView*)self.view;
    
    mesh = CreateMeshFromFile((char *)path.UTF8String);
    
    init(&context);
    context.screen_mat = &screen_mat;
    context.proj_mat = &proj_mat;
    context.mv_mat = &pos;
    
    set_screen_size(&context, self.novaView.bounds.size.width, self.novaView.bounds.size.height);
    set_hfov(&context, 60.0f);

    self.novaView.context = &context;
    
    self.current_time = CACurrentMediaTime();
    [NSTimer scheduledTimerWithTimeInterval:0.01f target:self selector:@selector(handleTimer:) userInfo:nil repeats:YES];
}

- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];
    
    // Update the view, if already loaded.
}

- (void) handleTimer:(NSTimer *)timer {
    CFTimeInterval new_current_time = CACurrentMediaTime();
    CFTimeInterval delta = self.current_time - new_current_time;
    self.current_time = new_current_time;
    
    MatSetRotY(&rot, ang);
    MatSetRotX(&rot2, ang / 2.0f);
    ang -= delta;
    MatSetTranslate(&trans, 0.0f, 0.0f, -5.f);
    MatMul(&rot2, &rot, &pos1);
    MatMul(&trans, &pos1, &pos);
    
    clear_pixel_buffer(&context);
    clear_depth_buffer(&context);
    
    render_mesh(&context, mesh);
    
    [self.view setNeedsDisplay:true];
}

@end
