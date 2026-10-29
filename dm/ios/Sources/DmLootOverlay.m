#import "DmLootOverlay.h"
#include "../../host/dm_loot_probe.h"
#include "../../host/dm_loot_live.h"
#include "../../host/dm_uiscreen.h"
#include "../../host/hle.h"
#include <stdlib.h>
#include <string.h>

UIFont *dm_loot_game_font(CGFloat size);
int dm_focus_now(void);

@implementation DmLootOverlay {
    dm_loot_owned_snapshot *_pending, *_drawn;
    unsigned char _eligible[DM_LOOT_MAX_LABELS];
    uint64_t _pendingEpoch, _drawnEpoch;
    BOOL _drawnReady;
}
- (instancetype)initWithFrame:(CGRect)frame {
    if (!(self=[super initWithFrame:frame])) return nil;
    self.opaque=NO; self.backgroundColor=UIColor.clearColor;
    self.userInteractionEnabled=NO; self.hidden=YES;
    _pending=calloc(1,sizeof *_pending); _drawn=calloc(1,sizeof *_drawn);
    return self;
}
- (void)dealloc { free(_pending); free(_drawn); }
- (void)reset {
    _drawnReady=NO; self.hidden=YES;
    dm_loot_live_invalidate();
}
- (void)tick:(uint64_t)now {
    if (!_pending || !_drawn || !self.window || self.bounds.size.width<2 ||
        self.bounds.size.height<2 || !gate_svc_present(SVC_HOST_LOOT_DRAW) ||
        !dm_focus_now() || !dm_uiscreen_ingame() || dm_uiscreen_panel()) {
        [self reset]; return;
    }
    // Only acknowledge a frame after drawRect completed and another display
    // tick arrived. This is not a GPU present fence; real-device validation is
    // still required. All unsupported labels retain the game's native draw.
    if (_drawnReady) {
        dm_loot_live_commit(_drawn,_eligible,_drawnEpoch,now);
        _drawnReady=NO;
    }
    uint64_t epoch=0;
    if (!dm_loot_probe_plan(_pending,&epoch)) { [self reset]; return; }
    _pendingEpoch=epoch; self.hidden=NO;
    [self setNeedsDisplay];
}
- (void)drawRect:(CGRect)dirty {
    (void)dirty;
    if (!_pending || !_drawn || !_pendingEpoch ||
        !dm_loot_probe_epoch_valid(_pendingEpoch)) { [self reset]; return; }
    int rw=0,rh=0;
    if (!dm_screen_real(&rw,&rh) || rw<=0 || rh<=0) { [self reset]; return; }
    CGFloat sx=self.bounds.size.width/(CGFloat)rw, sy=self.bounds.size.height/(CGFloat)rh;
    CGContextRef ctx=UIGraphicsGetCurrentContext();
    if (!ctx || sx<=0 || sy<=0) { [self reset]; return; }
    memset(_eligible,0,sizeof _eligible);
    for (unsigned i=0;i<_pending->geometry.count;i++) {
        const dm_loot_entry *e=&_pending->geometry.labels[i];
        const dm_loot_owned_label *l=&_pending->labels[i];
        NSString *s=[NSString stringWithUTF8String:l->text];
        // The original game renders multiline annotations in reverse order and
        // understands private-use inline colors. Leave them native until an
        // exact renderer is validated, rather than showing incorrect text.
        if (!s.length || [s rangeOfString:@"\ue07e"].location!=NSNotFound ||
            [s rangeOfString:@"\u00ffc"].location!=NSNotFound ||
            [s rangeOfString:@"\n"].location!=NSNotFound) continue;
        uint32_t bg[4]; memcpy(bg,l->style,sizeof bg);
        BOOL ok=YES; for(unsigned k=0;k<4;k++) if(bg[k]>255 || l->color[k]>255) ok=NO;
        if (!ok || !l->color[3]) continue;
        CGRect r=CGRectMake(e->rect.x*sx,e->rect.y*sy,e->rect.w*sx,e->rect.h*sy);
        if (CGRectIsEmpty(r) || !CGRectIntersectsRect(self.bounds,r)) continue;
        CGFloat size=MAX(6.0,MIN(48.0,r.size.height*0.72));
        UIFont *font=dm_loot_game_font(size);
        if (!font) continue;
        UIColor *fg=[UIColor colorWithRed:l->color[0]/255.0 green:l->color[1]/255.0
                                     blue:l->color[2]/255.0 alpha:l->color[3]/255.0];
        UIColor *back=[UIColor colorWithRed:bg[0]/255.0 green:bg[1]/255.0
                                       blue:bg[2]/255.0 alpha:bg[3]/255.0];
        NSDictionary *attr=@{NSFontAttributeName:font,NSForegroundColorAttributeName:fg};
        CGSize textSize=[s sizeWithAttributes:attr];
        if (textSize.width>r.size.width || textSize.height>r.size.height) continue;
        CGContextSaveGState(ctx);
        CGContextClipToRect(ctx,r);
        CGContextSetFillColorWithColor(ctx,back.CGColor); CGContextFillRect(ctx,r);
        CGPoint pt=CGPointMake(CGRectGetMidX(r)-textSize.width/2,
                               CGRectGetMidY(r)-textSize.height/2);
        [s drawAtPoint:pt withAttributes:attr];
        CGContextRestoreGState(ctx);
        _eligible[i]=1;
    }
    *_drawn=*_pending; _drawnEpoch=_pendingEpoch; _drawnReady=YES;
}
@end
