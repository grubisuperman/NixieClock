#ifndef _NeoPixelBusWrapper_h
#define _NeoPixelBusWrapper_h

#define colorSaturation 5

#include <functional> 
#include <memory>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

class NeoPixelBusWrapper
{
public:
    void init(void);
    void update(void);
    

    

private:
    const uint16_t PixelCount = 6;
    const uint8_t PixelPin = 3; // ignored for Esp8266
    const uint8_t AnimationChannels = 1;
    AnimationParam param;
    
    struct AnimationState
    {
        RgbColor StartingColor;
        RgbColor EndingColor;
    }animationState[0];

    bool fadeToColor = true;


    NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> stripe{PixelCount, PixelPin};
    NeoPixelAnimator animations{1,NEO_MILLISECONDS};

    //AnimationState animationState[1]; // TODO use initilizer list

    void BlendAnimUpdate(const AnimationParam& param);    
    void FadeInFadeOutRinseRepeat(float luminance);


    RgbColor red{10, 0, 0};
    RgbColor green{0, 10, 0};
    RgbColor blue{0, 0, 10};
    RgbColor white{10};
    RgbColor black{0};
};

#endif
