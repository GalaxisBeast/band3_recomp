#include "DataArray.h"
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <unordered_map>
#include <cstring>

#include "generated/band3_init.h"

// symbol --> func handler mapping for custom dta functions
static std::unordered_map<uint32_t, PPCFunc*> g_custom_dta_funcs;

// registers a custom DTA function by name
static void RegisterDTAFunc(PPCContext& ctx, uint8_t* base,
                            const char* name, PPCFunc* handler) {
    band3::Symbol sym(ctx, base, name);
    uint32_t sym_value = sym.value(base);

    if (!sym_value) {
        REXLOG_ERROR("RegisterDTAFunc: Symbol construction returned null for '{}'", name);
        return;
    }

    g_custom_dta_funcs[sym_value] = handler;
    REXLOG_INFO("Registered custom DTA function '{}' (sym={:08X})", name, sym_value);
}

// hook for DataArray::Execute, we check if the first node is a symbol (which is a rough indicator that we are trying to call a function) and pass it through to our handler
// kind of a hack but rexglue doesn't like calling indirect guest functions outside of the normal game code range or something and I can't figure out a cleaner way to do this
extern "C" void __imp__DataArray__Execute(PPCContext& ctx, uint8_t* base);
extern "C" REX_FUNC(DataArray__Execute) {
    uint32_t args_addr = ctx.r4.u32;
    uint32_t nodes_ptr = REX_LOAD_U32(args_addr);
    uint32_t first_type = REX_LOAD_U32(nodes_ptr + 4);
    uint32_t first_value = REX_LOAD_U32(nodes_ptr);

    if (first_type == band3::kDataSymbol) {
        auto it = g_custom_dta_funcs.find(first_value);
        if (it != g_custom_dta_funcs.end()) {
            it->second(ctx, base);
            return;
        }
    }

    __imp__DataArray__Execute(ctx, base);
}

static void ExitHandler(PPCContext& ctx, uint8_t* base) {
    REXLOG_INFO("Game requested exit, terminating title properly");
	
	// the proper way to terminate the title accoridng to Rexglue SDK
    rex::system::kernel_state()->TerminateTitle();
}

// register our custom DTA funcs after the game itself inits most of the DTA functions
extern "C" void __imp__DataInitFuncs(PPCContext& ctx, uint8_t* base);
extern "C" REX_FUNC(DataInitFuncs) {
    __imp__DataInitFuncs(ctx, base);
	
	// override exit to properly terminate the title
	// this will usually just crash things but this way we can properly handle this so in the future we can add a proper "Exit Game" button to main menu
	RegisterDTAFunc(ctx, base, "exit", ExitHandler);
	
	// custom functions should go here
}
