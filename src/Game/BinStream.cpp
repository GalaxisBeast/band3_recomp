#include "src/Game/BinStream.h"
#include <rex/hook.h>
#include <cstdint>
#include <cstring>

#ifdef _MSC_VER
#include <stdlib.h>
#define BSWAP16(x) _byteswap_ushort(x)
#define BSWAP32(x) _byteswap_ulong(x)
#define BSWAP64(x) _byteswap_uint64(x)
#else
#define BSWAP16(x) __builtin_bswap16(x)
#define BSWAP32(x) __builtin_bswap32(x)
#define BSWAP64(x) __builtin_bswap64(x)
#endif

extern "C" void __imp__BinStream__Read(PPCContext& ctx, uint8_t* base);
extern "C" void __imp__BinStream__Write(PPCContext& ctx, uint8_t* base);

// greasy inlined func to quickly cast to binstream
static inline band3::BinStream* GuestBinStream(uint8_t* base, uint32_t addr) {
    return reinterpret_cast<band3::BinStream*>(base + addr);
}

static inline void DoSwap(uint8_t* dst, const uint8_t* src, int32_t size) {
    switch (size) {
    case 2: {
        uint16_t val;
        std::memcpy(&val, src, 2);
        val = BSWAP16(val);
        std::memcpy(dst, &val, 2);
        break;
    }
    case 4: {
        uint32_t val;
        std::memcpy(&val, src, 4);
        val = BSWAP32(val);
        std::memcpy(dst, &val, 4);
        break;
    }
    case 8: {
        uint64_t val;
        std::memcpy(&val, src, 8);
        val = BSWAP64(val);
        std::memcpy(dst, &val, 8);
        break;
    }
    default:
        break;
    }
}

// SwapData(int* in, int* out, int size)
extern "C" REX_FUNC(SwapData)
{
    uint32_t in_addr = ctx.r3.u32;
    uint32_t out_addr = ctx.r4.u32;
    int32_t size = ctx.r5.s32;

    DoSwap(base + out_addr, base + in_addr, size);
}

// BinStream::ReadEndian(void* buf, size_t length)
extern "C" REX_FUNC(BinStream__ReadEndian)
{
    uint32_t this_addr = ctx.r3.u32;
    uint32_t buf_addr = ctx.r4.u32;
    int32_t length = ctx.r5.s32;

    __imp__BinStream__Read(ctx, base);

    if (GuestBinStream(base, this_addr)->littleEndian) {
        DoSwap(base + buf_addr, base + buf_addr, length);
    }
}

// BinStream::WriteEndian(void* data, int count)
extern "C" REX_FUNC(BinStream__WriteEndian)
{
    uint32_t this_addr = ctx.r3.u32;
    uint32_t data_addr = ctx.r4.u32;
    int32_t count = ctx.r5.s32;

    uint32_t write_addr = data_addr;

    uint32_t saved_r1 = ctx.r1.u32;
    ctx.r1.u32 = saved_r1 - 128;

    if (GuestBinStream(base, this_addr)->littleEndian) {
        uint32_t temp_addr = ctx.r1.u32 + 0x50;
        DoSwap(base + temp_addr, base + data_addr, count);
        write_addr = temp_addr;
    }

	// setup arguments
    ctx.r3.u64 = this_addr;
    ctx.r4.u64 = write_addr;
    ctx.r5.u64 = count;
    __imp__BinStream__Write(ctx, base);

    ctx.r1.u32 = saved_r1;
}
