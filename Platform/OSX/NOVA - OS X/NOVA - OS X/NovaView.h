//
//  NovaView.h
//  NOVA - OS X
//
//  Created by Andrew White on 1/28/16.
//  Copyright © 2016 Andrew White. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "../../../../Nova/nova_math.h"
#include "../../../../Nova/nova_render.h"

@interface NovaView : NSView

@property (nonatomic) struct RenderContext *context;

@end
