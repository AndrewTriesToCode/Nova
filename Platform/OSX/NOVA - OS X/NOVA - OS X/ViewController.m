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
@end

struct Matrix rot, rot2, trans, pos, pos1, screen_mat, proj_mat;
struct RenderContext context;
struct Mesh *mesh;
float ang = 0.0f;
CGImageRef image;
NSImage* nsImage;
CVDisplayLinkRef displayLink;
int frames;
NSTimeInterval secs;
NSTimeInterval secs2;


@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
        
    NSString *path = [[NSBundle mainBundle] pathForResource:  @"f16" ofType: @"obj"];
    
    mesh = CreateMeshFromFile((char *)path.UTF8String);
    
    init(&context);
    context.screen_mat = &screen_mat;
    context.proj_mat = &proj_mat;
    context.mv_mat = &pos;
    
    set_screen_size(&context, 1024, 1024);
    set_hfov(&context, 60.0f);

    ((NovaView*)self.view).context = &context;
    self.view.layer.delegate = self;
    
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    CVDisplayLinkSetOutputCallback(displayLink, &DisplayLinkCallback, (__bridge void * _Nullable)(self));
    CVDisplayLinkStart(displayLink);
    
    [NSTimer scheduledTimerWithTimeInterval:1.0f
    target:self selector:@selector(SetFPSTitle:) userInfo:nil repeats:YES];
}

- (void) SetFPSTitle:(NSTimer *)timer
{
    self.view.window.title = [NSString stringWithFormat:@"fps %d, render %.2f, draw %.2f, pct %.2f", frames, secs, secs2, (secs+secs2) / 0.16];
    frames = 0;
}

CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    NSViewController *controller = (__bridge NSViewController *)(displayLinkContext);
    [controller performSelectorOnMainThread:@selector(redraw) withObject:controller waitUntilDone:false];
    return COREVIDEO_TRUE;
}

- (void)redraw {
    
    if(self.view.inLiveResize)
        return;
    
    NSDate* start = [[NSDate alloc] init];

    MatSetRotY(&rot, ang);
    MatSetRotX(&rot2, ang / 2.0f);
    ang -= 0.005f;
    MatSetTranslate(&trans, 0.0f, 0.0f, -5.f);
    MatMul(&rot2, &rot, &pos1);
    MatMul(&trans, &pos1, &pos);
    
    clear_pixel_buffer(&context);
    clear_depth_buffer(&context);
    
        render_mesh(&context, mesh);
    secs = [start timeIntervalSinceNow] * -1000;

    [self.view setNeedsDisplay:true];
}

- (void)displayLayer:(CALayer *)theLayer {
    NSDate* start = [[NSDate alloc] init];

    void *src = get_pixel_buffer(&context);
    
    CFDataRef data = CFDataCreate(NULL, src, context.screen_width * context.screen_height * BYTES_PER_PIXEL);

    CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();

    CGImageRef image = CGImageCreate(context.screen_width, context.screen_height, 8, 32, context.screen_width * BYTES_PER_PIXEL, color_space, kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst, provider, NULL, YES, kCGRenderingIntentDefault);
    
    theLayer.contents = (__bridge id _Nullable)(image);
    
    CGImageRelease(image);
    CGColorSpaceRelease(color_space);
    CGDataProviderRelease(provider);
    CFRelease(data);
    frames++;
    secs2 = [start timeIntervalSinceNow] * -1000;

}
@end
