//
//  NovaView.m
//  NOVA - OS X
//
//  Created by Andrew White on 1/28/16.
//  Copyright © 2016 Andrew White. All rights reserved.
//

#import "NovaView.h"

@implementation NovaView

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    void *src = get_pixel_buffer(self.context);
    
    CFDataRef data = CFDataCreateWithBytesNoCopy(NULL, src, self.context->screen_width * self.context->screen_height * BYTES_PER_PIXEL, kCFAllocatorNull);
    
    CGDataProviderRef provider = CGDataProviderCreateWithCFData(data);
    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
    
    CGImageRef image = CGImageCreate(self.context->screen_width, self.context->screen_height, 8, 32, self.context->screen_width * BYTES_PER_PIXEL, color_space, kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst, provider, NULL, NO, kCGRenderingIntentDefault);
    
    CGContextRef context = [[NSGraphicsContext currentContext] graphicsPort];
    
    CGContextDrawImage(context, CGRectMake(0, 0, self.context->screen_width, self.context->screen_height), image);
    
    CGImageRelease(image);
    CGColorSpaceRelease(color_space);
    CGDataProviderRelease(provider);
}

@end
