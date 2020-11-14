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
    
    //if(self.inLiveResize)
    //    return;
    
    
}

- (void)viewDidEndLiveResize {
    [super viewDidEndLiveResize];
    
    //set_screen_size(self.context, self.bounds.size.width, self.bounds.size.height);
    //set_hfov(self.context, 60.0f);
}

@end
