#include "tray.hpp"

#import <Cocoa/Cocoa.h>
#include <atomic>
#include <memory>
#include <string>

@interface OSMVTrayTarget : NSObject
@property (nonatomic, assign) osmv::daemon::TrayCallbacks* callbacks;
- (void)onOpenClicked:(id)sender;
- (void)onQuitClicked:(id)sender;
@end

@implementation OSMVTrayTarget
- (void)onOpenClicked:(id)sender {
    if (self.callbacks && self.callbacks->on_show_gui) {
        self.callbacks->on_show_gui();
    }
}
- (void)onQuitClicked:(id)sender {
    if (self.callbacks && self.callbacks->on_quit) {
        self.callbacks->on_quit();
    }
}
@end

namespace osmv::daemon {

class MacOSTrayController final : public TrayController {
public:
    MacOSTrayController() {
        target_ = [[OSMVTrayTarget alloc] init];
    }

    ~MacOSTrayController() override {
        @autoreleasepool {
            if (status_item_) {
                [[NSStatusBar systemStatusBar] removeStatusItem:status_item_];
                [status_item_ release];
                status_item_ = nil;
            }
            if (menu_) {
                [menu_ release];
                menu_ = nil;
            }
            if (target_) {
                [target_ release];
                target_ = nil;
            }
        }
    }

    bool init(const TrayCallbacks& callbacks) override {
        callbacks_ = callbacks;
        target_.callbacks = &callbacks_;

        @autoreleasepool {
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

            status_item_ = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
            [status_item_ retain];

            if (status_item_.button) {
                status_item_.button.title = @"OSMV";
            }

            menu_ = [[NSMenu alloc] initWithTitle:@"OSMV"];

            item_open_ = [[NSMenuItem alloc] initWithTitle:is_fr_ ? @"Ouvrir OSMV" : @"Open OSMV"
                                                    action:@selector(onOpenClicked:)
                                             keyEquivalent:@""];
            [item_open_ setTarget:target_];
            [menu_ addItem:item_open_];

            [menu_ addItem:[NSMenuItem separatorItem]];

            item_quit_ = [[NSMenuItem alloc] initWithTitle:is_fr_ ? @"Quitter" : @"Quit"
                                                    action:@selector(onQuitClicked:)
                                             keyEquivalent:@"q"];
            [item_quit_ setTarget:target_];
            [menu_ addItem:item_quit_];

            [status_item_ setMenu:menu_];
        }
        return true;
    }

    void set_language(const std::string& lang) override {
        is_fr_ = (lang == "fr");
        if (item_open_ && item_quit_) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [item_open_ setTitle:is_fr_ ? @"Ouvrir OSMV" : @"Open OSMV"];
                [item_quit_ setTitle:is_fr_ ? @"Quitter" : @"Quit"];
            });
        }
    }

    void run_event_loop() override {
        running_.store(true);
        @autoreleasepool {
            [NSApp run];
        }
    }

    void request_quit() override {
        if (!running_.exchange(false)) return;
        dispatch_async(dispatch_get_main_queue(), ^{
            [NSApp stop:nil];
            NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                                location:NSMakePoint(0, 0)
                                           modifierFlags:0
                                               timestamp:0
                                            windowNumber:0
                                                 context:nil
                                                 subtype:0
                                                   data1:0
                                                   data2:0];
            [NSApp postEvent:event atStart:YES];
        });
    }

private:
    TrayCallbacks callbacks_;
    OSMVTrayTarget* target_ = nil;
    NSStatusItem* status_item_ = nil;
    NSMenu* menu_ = nil;
    NSMenuItem* item_open_ = nil;
    NSMenuItem* item_quit_ = nil;
    bool is_fr_ = false;
    std::atomic<bool> running_{false};
};

std::unique_ptr<TrayController> create_tray_controller() {
    return std::make_unique<MacOSTrayController>();
}

} // namespace osmv::daemon
