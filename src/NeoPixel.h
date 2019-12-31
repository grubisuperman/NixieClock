#ifndef _NeoPixel_h
#define _NeoPixel_h

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

namespace NeoPixel
{
const uint16_t PixelCount = 6;
const uint8_t PixelPin = 3; // ignored for Esp8266
const uint8_t AnimationChannels = 2;


enum AnimSelect
{
    none,
    CyclonEye,
    NeoPixelFunRandomChange,
    Standby
};

NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip{PixelCount, PixelPin};
NeoPixelAnimator animations{2, NEO_MILLISECONDS};

AnimationParam param;

//NeoPixelFunRandomChange
struct AnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
};

// one entry per pixel to match the animation timing manager
AnimationState animationState[AnimationChannels];

// local vars to track animation
bool fadeToColor = true;

// Cylon Red Eye
RgbColor rgbLEDColor;

uint16_t lastPixel = 0; // track the eye position
int8_t moveDir = 1;     // track the direction of movement
// uncomment one of the lines below to see the effects of
// changing the ease function on the movement animation
AnimEaseFunction moveEase =
    //      NeoEase::Linear;
    //      NeoEase::QuadraticInOut;
    //      NeoEase::CubicInOut;
    NeoEase::QuarticInOut;
//      NeoEase::QuinticInOut;
//      NeoEase::SinusoidalInOut;
//      NeoEase::ExponentialInOut;
//      NeoEase::CircularInOut;

//#######################
//      NeoPixelFunRandomChange
//#######################

void BlendAnimUpdate(const AnimationParam &param)
{
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbColor updatedColor = RgbColor::LinearBlend(
        animationState[param.index].StartingColor,
        animationState[param.index].EndingColor,
        param.progress);

    // apply the color to the strip
    for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
    {
        strip.SetPixelColor(pixel, updatedColor);
    }
}

void FadeInFadeOutRinseRepeat(float luminance)
{
    if (fadeToColor)
    {
        // Fade upto a random color
        // we use HslColor object as it allows us to easily pick a hue
        // with the same saturation and luminance so the colors picked
        // will have similiar overall brightness
        RgbColor target = HslColor(random(360) / 360.0f, 1.0f, luminance);
        uint16_t time = random(3000, 6000);

        animationState[0].StartingColor = strip.GetPixelColor(0);
        animationState[0].EndingColor = target;

        animations.StartAnimation(0, time, BlendAnimUpdate);
    }
    else
    {
        // fade to black
        uint16_t time = random(3000, 6000);

        animationState[0].StartingColor = strip.GetPixelColor(0);
        animationState[0].EndingColor = RgbColor(0);

        animations.StartAnimation(0, time, BlendAnimUpdate);
    }

    // toggle to the next effect state
    fadeToColor = !fadeToColor;
}

//#######################
//      CYCLON EYE
//#######################
void FadeAll(uint8_t darkenBy)
{
    RgbColor color;
    for (uint16_t indexPixel = 0; indexPixel < strip.PixelCount(); indexPixel++)
    {
        color = strip.GetPixelColor(indexPixel);
        color.Darken(darkenBy);
        strip.SetPixelColor(indexPixel, color);
    }
}

void FadeAnimUpdate(const AnimationParam &param)
{
    if (param.state == AnimationState_Completed)
    {
        FadeAll(10);
        animations.RestartAnimation(param.index);
    }
}

void MoveAnimUpdate(const AnimationParam &param)
{
    // apply the movement animation curve
    float progress = moveEase(param.progress);

    // use the curved progress to calculate the pixel to effect
    uint16_t nextPixel;
    if (moveDir > 0)
    {
        nextPixel = progress * PixelCount;
    }
    else
    {
        nextPixel = (1.0f - progress) * PixelCount;
    }

    // if progress moves fast enough, we may move more than
    // one pixel, so we update all between the calculated and
    // the last
    if (lastPixel != nextPixel)
    {
        for (uint16_t i = lastPixel + moveDir; i != nextPixel; i += moveDir)
        {
            strip.SetPixelColor(i, rgbLEDColor);
        }
    }
    strip.SetPixelColor(nextPixel, rgbLEDColor);

    lastPixel = nextPixel;

    if (param.state == AnimationState_Completed)
    {
        // reverse direction of movement
        moveDir *= -1;

        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);
    }
}

void SetupCyclonEyeAnimations()
{
    // fade all pixels providing a tail that is longer the faster
    // the pixel moves.
    animations.StartAnimation(0, 100, FadeAnimUpdate);

    // take several seconds to move eye fron one side to the other
    animations.StartAnimation(1, 2000, MoveAnimUpdate);
}

//#######################
//      COMMON
//#######################
void setup()
{
    strip.Begin();
    strip.Show();
}

void update(AnimSelect targetAnimation)
{
    static AnimSelect lastAnimRun;
    byte runAnim;
    // derive colorStandby from rgbLEDColor
    static HsbColor colorStandby = HsbColor(rgbLEDColor);
    // set reduced brightness
    colorStandby.B = 0.005;
        
    // target animation has changed. Reset animations and run setup of target
    if (lastAnimRun != targetAnimation)
    {
        runAnim = targetAnimation + 10;
        animations.StopAll();
    }
    else // run animation
    {
        runAnim = targetAnimation;
    }
    

    

    switch (runAnim)
    {

    // None    
    case none:
        for (uint16_t indexPixel = 0; indexPixel < strip.PixelCount(); indexPixel++)
        {
            strip.SetPixelColor(indexPixel,rgbLEDColor);
        }
        strip.Show();
        break;

    case (none+10):
        lastAnimRun = targetAnimation;
        break;


    // NeoPixelFunRandomChange
    case NeoPixelFunRandomChange:
        if (animations.IsAnimating())
        {
            // the normal loop just needs these two to run the active animations
            animations.UpdateAnimations();
            strip.Show();
        }
        else
        {
            // no animation runnning, start some
            
            FadeInFadeOutRinseRepeat(HslColor(rgbLEDColor).L); // 0.0 = black, 0.25 is normal, 0.5 is bright
        }
        break;

    // Setup NeoPixelFunRandomChange
    case (NeoPixelFunRandomChange + 10):
        lastAnimRun = targetAnimation;
        break;


    // CyclonEye
    case CyclonEye:
        animations.UpdateAnimations();
        strip.Show();
        break;
    
    // Setup CyclonEye
    case (CyclonEye + 10):
        SetupCyclonEyeAnimations();
        lastAnimRun = targetAnimation;
        break;

    // Standby   
    case Standby:
                
        for (uint16_t indexPixel = 0; indexPixel < strip.PixelCount(); indexPixel++)
        {
            strip.SetPixelColor(indexPixel,colorStandby);
        }
        strip.Show();
        break;

    case (Standby + 10):
        lastAnimRun = targetAnimation;
        break;

    default:
        break;
    }
}

} // namespace NeoPixel

#endif