# GreenSection
Nvidia GreenSection Memory Corruption 0day vulnerability

Multiple NVIDIA user mode components shares a global memory section in \\BaseNamedObjects\\{52813408-3561-4705-820a-2b3b78be92ba} with full R/W access to everyone, this section stores important data structures inside but there are checks at execution times that prevent anything going wrong.
Unfortunately the same data from the section are re-used at runtime causing out-of-bound memory write.

While this bug does not get SYSTEM privileges immediately but it can be used cross user to user boundary easily or even compromise the dwm.exe process, I didn't look deeply into it but I'd be happy to see someone making a full exploit out of it.

Reproduction : 
1. Run any app that uses vulkan or OpenGL
2. Run the PoC and press enter
3. Observe the crash

```
STACK_TEXT:  
00000089`75bfed00 00007ff9`7564e3a4     : 00000000`00000001 00000000`00000001 000001da`b5201340 00000000`00000000 : nvoglv64!DrvPresentBuffers+0xbcdf8
00000089`75bfed30 00007ff9`7564b62a     : 000001da`b7910080 00000000`00000000 000001da`b5201301 00000000`00000009 : nvoglv64!DrvPresentBuffers+0xbbc84
00000089`75bfefe0 00007ff9`7592d738     : 00000000`00000000 00000000`00001760 00000000`00000000 000001da`b58a1e80 : nvoglv64!DrvPresentBuffers+0xb8f0a
00000089`75bff020 00007ff9`7592dc6b     : 000001da`00000000 000001da`b7910080 00000000`00000000 00000089`75bff150 : nvoglv64!vk_gr2608GetInstanceProcAddr+0x3a5e8
00000089`75bff080 00007ff9`75acd90a     : 00000089`75bff300 00000000`00000000 000001da`b52d1480 00000000`00000000 : nvoglv64!vk_gr2608GetInstanceProcAddr+0x3ab1b
00000089`75bff0f0 00007ff9`6eca26dd     : 000001db`3ef2a000 00000089`75bff300 000001da`00000400 00006a2d`00000000 : nvoglv64!vkGetInstanceProcAddr+0x16698a
00000089`75bff2e0 00007ff9`6ed1c086     : 00000000`04bf151e 00000000`00000000 00000000`00000000 00000000`04bf151e : plugin_gxc_vulkan2_x64!ImGuiTextFilter::`default constructor closure'+0x11e3d
00000089`75bff370 00007ff9`6ed15239     : 00000000`00000000 000001da`f3b548b0 000001da`f3b2d3f0 00000000`00000000 : gpumagick_sdk_x64!gm::app_set_param_bool+0xbb5e
00000089`75bff770 00007ffa`3aede8d7     : 00000000`00000000 00000000`00000000 00000000`00000000 00000000`00000000 : gpumagick_sdk_x64!gm::app_set_param_bool+0x4d11
00000089`75bff7b0 00007ffa`3bccc48c     : 00000000`00000000 00000000`00000000 000004f0`fffffb30 000004d0`fffffb30 : kernel32!BaseThreadInitThunk+0x17
00000089`75bff7e0 00000000`00000000     : 00000000`00000000 00000000`00000000 00000000`00000000 00000000`00000000 : ntdll!RtlUserThreadStart+0x2c
```
