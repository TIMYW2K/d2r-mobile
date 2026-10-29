#import <UIKit/UIKit.h>
#include <stdint.h>
// Experimental 60 Hz layer. Never receives touch events itself.
@interface DmLootOverlay : UIView
- (void)tick:(uint64_t)now;
- (void)reset;
@end
