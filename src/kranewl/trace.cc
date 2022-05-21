#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>

#include <cstdio>
#include <iomanip>

namespace {
    Dl_info get_dl_info(void*,  void*) __attribute__((no_instrument_function));

    Dl_info get_dl_info(void*func,  void*) {
        Dl_info info;
        if(!dladdr(func, &info)){
            info.dli_fname = "?";
            info.dli_sname = "?";
        }
        if(!info.dli_fname) {
            info.dli_fname = "?";
        }
        if(!info.dli_sname) {
            info.dli_sname = "?";
        }
        return info;
    }
}

extern "C" {
    void __cyg_profile_func_enter(void*, void*) __attribute__((no_instrument_function));
    void __cyg_profile_func_exit(void*, void*)  __attribute__((no_instrument_function));

    void __cyg_profile_func_enter(void* func,  void* caller) {
        auto info = get_dl_info(func, caller);
        std::printf("> %p [%s] %s\n", func, info.dli_fname, info.dli_sname);
    }

    void __cyg_profile_func_exit (void* func, void* caller) {
        auto info = get_dl_info(func, caller);
        std::printf("< %p [%s] %s\n", func, info.dli_fname, info.dli_sname);
    }
};
