#include <rex/logging.h>
#include "generated/band3_init.h"
#include "src/config.h"

extern "C" void __imp__BoxMapLighting__ApplyQueuedLights(PPCContext& ctx, uint8_t* base);
extern "C" void __imp__RndMat__Load(PPCContext& ctx, uint8_t* base);
extern "C" void __imp__OutfitConfig__CompressTextures(PPCContext& ctx, uint8_t* base);

extern "C" REX_FUNC(BoxMapLighting__ApplyQueuedLights)
{
    if (band3::GetConfig().disable_approximate_lights) {
        return;
    }
    __imp__BoxMapLighting__ApplyQueuedLights(ctx, base);
}

extern "C" REX_FUNC(RndMat__Load)
{
    uint32_t this_addr = ctx.r3.u32;
    __imp__RndMat__Load(ctx, base);
    if (band3::GetConfig().disable_hair_shader) {
        uint32_t shader = REX_LOAD_U32(this_addr + 0x118);
        if (shader == 2) {
			// set shader variation to kShaderVariationNone
            REX_STORE_U32(this_addr + 0x118, 0);
        }
    }
	
    if (band3::GetConfig().fullbright) {
		// force useEnviron to be 0
        REX_STORE_U8(this_addr + 0x99, 0);
    }
}

extern "C" void __imp__ProcCounter__ProcCommands(PPCContext& ctx, uint8_t* base);
extern "C" REX_FUNC(ProcCounter__ProcCommands)
{
    if (band3::GetConfig().disable_even_odd_rendering) {
        ctx.r3.u64 = 7;
        return;
    }
    __imp__ProcCounter__ProcCommands(ctx, base);
}

extern "C" REX_FUNC(OutfitConfig__CompressTextures)
{
    if (!band3::GetConfig().compress_character_textures) {
        return;
    }
    __imp__OutfitConfig__CompressTextures(ctx, base);
}
