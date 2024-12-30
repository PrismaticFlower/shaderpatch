#include "framework.hpp"

#include "user_config_descriptions.hpp"

using namespace std::literals;

auto load_user_config_descriptions()
   -> std::unordered_map<std::wstring_view, std::wstring_view>
{
   return {
      // clang-format off

{
L"Display"sv,      
LR"(Settings for controlling the display of the game.)"sv
},

{
L"Allow Tearing"sv,      
LR"(Allow tearing when displaying frames. This can allow lower latency and is also required for variable refresh rate technologies to work.)"sv
},

{
L"Treat 800x600 As Interface"sv,      
LR"(Whether 800x600 will be treated as the resolution used by the game's main menu interface. When enabled causes Shader Patch to override the game's resolution without informing the game it has done so. Doing this keeps the interface usable while still letting you enjoy fullscreen ingame.)"sv
},

{
L"Stretch Interface"sv,      
LR"(When Treat 800x600 As Interface is enabled controls if the interface will be stretched across the screen or if it's 4:3 aspect ratio will be maintained using black bars.)"sv
},

{
L"Display Scaling Aware"sv,      
LR"(Whether to mark the game DPI aware or not. When your display scaling is not at 100% having this be `yes` keeps the game being drawn at native resolution and stops Windows' upscaling it after the fact.)"sv
},

{
L"Display Scaling"sv,      
LR"(When `Display Scaling Aware` is `yes` controls if the game's perceived resolution will be scaled based on the display scaling factor. This effectively adjusts the scale UI and HUD elements are positioned and drawn with to match the display's scale.)"sv
},

{
L"DSR-VSR Display Scaling"sv,      
LR"(When `Display Scaling Aware` is `yes` controls if the game's perceived resolution will be scaled based on if Shader Patch detects Dynamic Super Resolution/Virtual Super Resolution being used. The detection is based on if the game's resolution increases past the starting desktop resolution.)"sv
},

{
L"Scalable Fonts"sv,      
LR"(Whether to replace the game's core fonts with ones that can be scaled. If you have any mods installed that also replace the game's fonts when this setting is enabled then they will be superseded by Shader Patch's fonts.)"sv
},

{
L"Aspect Ratio Hack"sv,      
LR"(Enables a "hack" (or series of) to override the game's aspect ratio while keeping the HUD visible. It's not perfect but can make the game far more playable at ultrawide aspect ratios than it otherwise would be.

This feature will search the memory around the "D3DPRESENT_PARAMETERS" it passes to "IDirect3DDevice9::Reset" for the aspect ratio. It is able to successfully find it on all versions of the game I have (DVD/GoG/Steam/Modtools) and no other values in the area seem to resemble anything looking like an aspect ratio. Which should make it quite reliable. But there is always the tiny chance it'll end up overwriting some random pointer and cause all sorts of fun (hopefully just a crash but who knows), so use at your own risk and all that. This is the only feature in SP to depend on reading or writing a value in the game's memory. And it will NEVER do it unless this is explicitly enabled.

Loading screens will often appear distorted when using this.

See and configure Aspect Ratio Hack HUD Handling below as well.)"sv
},

{
L"Aspect Ratio Hack HUD Handling"sv,      
LR"(How to handle the HUD when using the Aspect Ratio Hack. Can be "Stretch 4_3", "Centre 4_3", "Stretch 16_9" or "Centre 16_9".

In Stretch mode the HUD will be stretched out across the entire screen. This can look acceptable depending on the amount of stretching, a 4:3 HUD stretched to 32:9 will look worse than a 16:9 HUD stretched to the same.

In Centre mode the HUD will be drawn into the centre of the screen with the correct aspect ratio. Mostly, some elements like the crosshair ammo counter and weapon heat will be misaligned and potentially stretched.)"sv
},

{
L"Enable Game Perceived Resolution Override"sv,      
LR"(Enables the overriding the resolution the game thinks it is rendering at, without changing the resolution it is actually rendered at. This can have the affect of altering the aspect ratio of the game and the scaling and position of UI/HUD elements.

When set to `yes` causes `Display Scaling` to be ignored.

If you're unsure what this is useful for then **leave** it set to "no".)"sv
},


{
L"Game Perceived Resolution Override"sv,      
LR"(The override for the game's perceived resolution, for use with `Enable Game Perceived Resolution Override`.)"sv
},

{
L"Override Resolution"sv,      
LR"(Enables the overriding the resolution the game will be rendered at to a percentage of the monitor's resolution. This is mostly intended for use with modtools' debug .exe.)"sv
},

{
L"Override Resolution Screen Percent"sv,      
LR"(When Override Resolution is enabled the percentage of the screen the game's window will take up.)"sv
},

{
L"User Interface"sv,      
LR"(Settings affecting the user interface/HUD of the game.

Note that Friend-Foe color changes do not work when a unit flashes on the minimap. They do work for everything else.)"sv
},

{
L"Extra UI Scaling"sv,      
LR"(Extra scaling for the UI/HUD on top of normal display scaling. As a percentage. This can be used to make text larger if it is too small but only has effect when Display Scaling is enabled.

Note that the trick Shader Patch uses to scale the game's UI (lowering the resolution the game thinks it's rendering at) can't scale indefinitely, if changing this has to effect and you're on a high DPI screen it's possible you've hit the limit for how much scaling Shader Patch can allow.)"sv
},

{
L"Friend Color"sv,      
LR"(Color to use for Friend HUD and world elemenets. (Command Posts, crosshair, etc))"sv
},

{
L"Foe Color"sv,      
LR"(Color to use for Foe HUD and world elemenets. (Command Posts, crosshair, etc))"sv
},

{
L"Graphics"sv,      
LR"(Settings directly affecting the rendering of the game.)"sv
},

{
L"GPU Selection Method"sv,      
LR"(How to select the GPU to use. Can be "Highest Performance", "Lowest Power Usage", "Highest Feature Level", "Most Memory" or "Use CPU".)"sv
},

{
L"Anti-Aliasing Method"sv,      
LR"(Anti-Aliasing method to use, can be "none", "CMAA2", "MSAAx4" or "MSAAx8".)"sv
},

{
L"Supersample Alpha Test"sv,      
LR"(Enables supersampling the alpha test for hardedged transparency/alpha cutouts when Multisampling Anti-Aliasing is enabled.

This provides excellent anti-aliasing for alpha cutouts (foliage, flat fences, etc) and should be much cheaper than general supersampling. It however does raise the number of texture samples that need to be taken by however many rendertarget samples there are - fast high end GPUs may not even notice this, low power mobile GPUs may struggle more.

If enabling this causes performance issues but you'd still really like it then lowering the level of Anisotropic Filtering may help make reduce the performance cost of this option.)"sv
},

{
L"Anisotropic Filtering"sv,      
LR"(Anisotropic Filtering level for textures, can be "off", "x2", "x4", "x8" or "x16".)"sv
},

{
L"Enable 16-Bit Color Channel Rendering"sv,      
LR"(Controls whether R1G16B16A16 rendertargets will be used or not. Due to the way SWBFII is drawn using them can cut down significantly on banding. Additionally when they are enabled dithering will be applied when they are copied into R8G8B8A8 textures for display to further reduce banding.

If Effects are enabled and HDR rendering is enabled by a map then this setting has no effect.)"sv
},

{
L"Enable Order-Independent Transparency"sv,      
LR"(Enables the use of an Order-Independent Transparency approximation. It should go without saying that this increases the cost of rendering. Requires a GPU with support for Rasterizer Ordered Views and typed Unordered Access View loads. 

When this is enabled transparent objects will no longer be affected by Multisample Anti-Aliasing (MSAA), this can be especially noticeable when using resolution scaling - although it does depend on the map. Postprocessing Anti-Aliasing options like CMAA2 are unaffected.

Known to be buggy on old NVIDIA drivers. Try updating your GPU driver if you encounter issues.)"sv
},

{
L"Enable Alternative Post Processing"sv,      
LR"(Enables the use of an alternative post processing path for some of the game's post processing effects. The benefit to this is the alternative path is designed to be independent of the game's resolution and is unaffected by bugs the vanilla path suffers from relating to high resolutions.

In some cases the alternative path may produce results that do not exactly match the intended look of a map.)"sv
},

{
L"Allow Vertex Soft Skinning"sv,      
LR"(Allows the use of Shader Patch's soft skinning shaders. This makes compatible assets appear smoother when being animated.

There is a very, very small chance of compatibility issues from using this. If in the unlikely event you get unusual looking player models with Shader Patch (but not without) then turning this off may solve it.)"sv
},

{
L"Enable Scene Blur"sv,      
LR"(Enables maps to make use of the game's scene blur effect. Has a performance cost but using it will keep the map's artwork more inline with the creators vision. Note this is **unrelated** to the Effects system and also only takes effect when `Enable Alternative Post Processing` is `yes`.)"sv
},

{
L"Refraction Quality"sv,      
LR"(Quality level of refractions. Can be "Low", "Medium", "High" or "Ultra".
"Low" sets the resolution of the refractions to 1/4 screen resolution, Medium 1/2, High 1/2 and Ultra matches screen resolution.

On "High" or "Ultra" refractions will also be masked using the depth buffer.)"sv
},

{
L"Disable Light Brightness Rescaling"sv,      
LR"(In the vanilla shaders for most surfaces after calculating incoming light for a surface the brightness of the light is rescaled to keep it from being too dark or too bright. (Or at least that is what I *assume* the reason to be.) 
 
Disabling this can increase the realism and correctness of lighting for objects lit by the affected shaders. But for maps/mods that have had their lighting carefully balanced by the author it could do more harm than good. It is also likely to increase the amount of surfaces that are bright enough to cause light bloom in a scene.

This setting has no affect on custom materials or when Effects are enabled. (Effects have an equivalent setting however.))"sv
},

{
L"Enable User Effects Config"sv,      
LR"(When a map or mod has not loaded an Effects Config Shader Patch can use a user specified config.)"sv
},

{
L"User Effects Config"sv,      
LR"(The name of the Effects Config to load when `Enable User Effects Config` is true. The config should be in the same directory as the game's executable.)"sv
},

{
L"Use Direct3D 11 on 12"sv,      
LR"(Force using Direct3D 11 on 12 instead of the any native Direct3D 11 driver. This can workaround bugs in the D3D11 driver. It is likely that turning this on will cost some performance however. CMAA2 is also broken while using this (unsure why, there are no debug layer errors from D3D11 or D3D12 about it).)"sv
},

{
L"Effects"sv,      
LR"(Settings for the Effects system, which allows modders to apply various effects to their mods at their discretion and configuration. Below are options provided to tweak the performance of this system for low-end/older GPUs.)"sv
},

{
L"Bloom"sv,      
LR"(Enable or disable a mod using the Bloom effect. This effect is can have a slight performance impact.)"sv
},

{
L"Vignette"sv,      
LR"(Enable or disable a mod using the Vignette effect. This effect is really cheap.)"sv
},

{
L"Film Grain"sv,      
LR"(Enable or disable a mod using the Film Grain effect. This effect is fairly cheap.)"sv
},

{
L"Allow Colored Film Grain"sv,      
LR"(Enable or disable a mod using being allowed to use Colored Film Grain. Slightly more expensive than just regular Film Grain, if a mod is using it.)"sv
},

{
L"SSAO"sv,      
LR"(Enable or disable a mod using Screen Space Ambient Occlusion. (Not actually true Ambient Occlusion as it is applied indiscriminately to opaque surfaces.)

Has anywhere from a slight performance impact to significant performance impact depending on the quality setting.)"sv
},

{
L"SSAO Quality"sv,      
LR"(Quality of SSAO when enabled. Can be "Fastest", "Fast", Medium", "High" or "Highest". Each step up is usually 1.5x-2.0x more expensive than the last setting.)"sv
},

{
L"Developer"sv,      
LR"(Settings for both mod makers and Shader Patch developers. If you don't fall into those groups these settings likely aren't useful.)"sv
},

{
L"Screen Toggle"sv,      
LR"(Toggle key for the developer screen. Giving access to useful things for mod makers and Shader Patch developers alike.

The value identifies the virtual key that activates the debug screen, below are some common values for you to choose from. (Copy and paste the value on the right of your desired key.)

'~': 0xC0 
'\': 0xDC 
'Backspace': 0x08 
'F12': 0x7B)"sv
},

{
L"Monitor BFront2.log"sv,      
LR"(Enables the monitoring, filtering and display of "BFront2.log" ingame. Can not be changed ingame. Only useful to modders.)"sv
},

{
L"Allow Event Queries"sv,      
LR"(Controls whether or not Shader Patch will allow the game to use GPU event queries.)"sv
},

{
L"Use D3D11 Debug Layer"sv,      
LR"(Enable the D3D11 debug layer, typically requires the Windows SDK to be installed to work.

Even if this flag is true the debug layer will only be enabled if a debugger is attached to the game.)"sv
},

{
L"Use DXGI 1.2 Factory"sv,      
LR"(Limit Shader Patch to using a DXGI 1.2 factory to work around a crash in the Visual Studio Graphics Debugger. This also bypasses all GPU selection logic.)"sv
},

{
L"Shader Cache Path"sv,      
LR"(Path for shader cache file.)"sv
},

{
L"Shader Definitions Path"sv,      
LR"(Path to shader definitions. (.json files describing shader entrypoints and variations within a file))"sv
},

{
L"Shader Source Path"sv,      
LR"(Path to shader HLSL source files.)"sv
},

{
L"Material Scripts Path"sv,      
LR"(Path to material types Lua scripts.)"sv
},

{
L"Scalable Font Name"sv,      
LR"(Name of the font to use when Scalable Fonts are enabled.)"sv
},
      // clang-format on
   };
}
