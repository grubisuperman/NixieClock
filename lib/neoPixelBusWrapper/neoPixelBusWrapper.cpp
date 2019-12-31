#include "neoPixelBusWrapper.h"

void NeoPixelBusWrapper::init(void)
{
    stripe.Begin();
    stripe.Show();
}

void NeoPixelBusWrapper::update(void)
{
    if (animations.IsAnimating())
    {
        // the normal loop just needs these two to run the active animations
        animations.UpdateAnimations();
        Serial.print("param.index = ");
        Serial.println(param.index);
        Serial.print("param.progress = ");
        Serial.println(param.progress);
        Serial.print("param.state = ");
        Serial.println(param.state);  
        stripe.Show();
    }
    else
    {
        // no animation runnning, start some 
        //
        Serial.println(F("neoPixel: start animation"));
        FadeInFadeOutRinseRepeat(0.2f); // 0.0 = black, 0.25 is normal, 0.5 is bright
    }
}

void NeoPixelBusWrapper::BlendAnimUpdate(const AnimationParam& param)
{
    //Serial.println("neoPixel: BlendAnimUpdate");
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbColor updatedColor = RgbColor::LinearBlend(
        animationState[0].StartingColor,
        animationState[0].EndingColor,
        param.progress);
        //Serial.println(param.progress);


    // apply the color to the strip
    for (uint16_t pixel = 0; pixel < PixelCount; pixel++)
    {
        stripe.SetPixelColor(pixel, updatedColor);
    }
}

void NeoPixelBusWrapper::FadeInFadeOutRinseRepeat(float luminance)
{
    if (fadeToColor)
    {
        // Fade upto a random color
        // we use HslColor object as it allows us to easily pick a hue
        // with the same saturation and luminance so the colors picked
        // will have similiar overall brightness
        RgbColor target = HslColor(random(360) / 360.0f, 1.0f, luminance);
        uint16_t time = random(800, 2000);
        //uint16_t time = 6000;

        animationState[0].StartingColor = stripe.GetPixelColor(0);
        animationState[0].EndingColor = target;

        animations.StartAnimation(0, time, std::bind(&NeoPixelBusWrapper::BlendAnimUpdate,this,&param));
    }
    else 
    {
        // fade to black
        uint16_t time = random(600, 700);
        animationState[0].StartingColor = stripe.GetPixelColor(0);
        animationState[0].EndingColor = RgbColor(0);
        //f_cb = std::bind(&NeoPixelBusWrapper::BlendAnimUpdate,this,param);
        animations.StartAnimation(0, time, std::bind(&NeoPixelBusWrapper::BlendAnimUpdate,this,NeoPixelBusWrapper::param));
    }

    // toggle to the next effect state
    fadeToColor = !fadeToColor;
}
