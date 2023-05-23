# VulkanAsio
Asio enabled Vulkan app for guitar. WIP.<br/>
You need an Asio driver (Windows I guess) and audio interface.<br/>
Lot's of bit and pieces, various effects, save sound to file (saves to save.wave for now), click beat (snare drum, but changeable, one day it might grow up to be a metronome), nothing complete.<br/>
If I ever continue it, I'll have some form of layout of "pedals" shown in the order you select the effects.
This is very ugly at the moment, but the audio processing/Asio interface works, sort of. <br/>
The audio processing (fxobjects.h) code is NOT mine but Will Pirkles. <br/>
Available at: https://www.willpirkle.com/audiofxlab/ <br/>
Asio code based on Steinbergs API.<br/>
Available at: https://www.steinberg.net/developers/ <br/>
Dear ImGui ui, with knob extension.<br/>
https://github.com/ocornut/imgui<br/>
https://github.com/altschuler/imgui-knobs<br/>
Raw wave files from stk.
https://github.com/thestk/stk
