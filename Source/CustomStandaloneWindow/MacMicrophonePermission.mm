#include <functional>
#include <memory>

#include <juce_core/juce_core.h>

#import <AVFoundation/AVFoundation.h>
#import <dispatch/dispatch.h>

namespace juce
{

void requestMacOSMicrophoneAccess(std::function<void()> completion);

void requestMacOSMicrophoneAccess(std::function<void()> completion)
{
    if (@available (macOS 10.14, *))
    {
        const auto status = [AVCaptureDevice authorizationStatusForMediaType: AVMediaTypeAudio];

        if (status == AVAuthorizationStatusNotDetermined)
        {
            auto callback = std::make_shared<std::function<void()>> (std::move (completion));

            [AVCaptureDevice requestAccessForMediaType: AVMediaTypeAudio
                                     completionHandler: ^(BOOL granted)
            {
                ignoreUnused (granted);
                dispatch_async (dispatch_get_main_queue(), ^
                {
                    if (*callback)
                        (*callback)();
                });
            }];

            return;
        }
    }

    if (completion)
        completion();
}

} // namespace juce
