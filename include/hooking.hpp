#pragma once

#include "beatsaber-hook/shared/utils/hooking.hpp"

namespace SongCore {
    struct Hooking {
        using InstallFunc_t = void(*)(Logger& logger);
        private:
            inline static std::vector<InstallFunc_t> installFuncs;
        public:
            static void AddInstallFunc(InstallFunc_t installFunc) {
                if (std::find(installFuncs.begin(), installFuncs.end(), installFunc) == installFuncs.end())
                    installFuncs.emplace_back(installFunc);
            }

            static void InstallHooks(Logger& logger) {
                for (auto func : installFuncs) {
                    func(logger);
                }
            }
    };
}

#define HOOK_AUTO_INSTALL_ORIG(name_)                                                                                  \
    struct Auto_Hook_Install_##name_ {                                                                                 \
        Auto_Hook_Install_##name_() { ::SongCore::Hooking::AddInstallFunc(::Hooking::InstallOrigHook<Hook_##name_>); } \
    };                                                                                                                 \
    static Auto_Hook_Install_##name_ Auto_Hook_Install_##name_##_Instance

#define HOOK_AUTO_INSTALL(name_)                                                                                   \
    struct Auto_Hook_Install_##name_ {                                                                             \
        Auto_Hook_Install_##name_() { ::SongCore::Hooking::AddInstallFunc(::Hooking::InstallHook<Hook_##name_>); } \
    };                                                                                                             \
    static Auto_Hook_Install_##name_ Auto_Hook_Install_##name_##_Instance

#define MAKE_AUTO_HOOK_MATCH(name_, mPtr, retval, ...)                                                                                              \
    struct Hook_##name_ {                                                                                                                           \
        using funcType = retval (*)(__VA_ARGS__);                                                                                                   \
        static_assert(std::is_same_v<funcType, ::Hooking::InternalMethodCheck<decltype(mPtr)>::funcType>, "Hook method signature does not match!"); \
        constexpr static const char* name() { return #name_; }                                                                                      \
        static const MethodInfo* getInfo() { return ::il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::methodInfo(); }                        \
        static funcType* trampoline() { return &name_; }                                                                                            \
        static inline retval (*name_)(__VA_ARGS__) = nullptr;                                                                                       \
        static funcType hook() { return &::Hooking::HookCatchWrapper<&hook_##name_, funcType>::wrapper; }                                           \
        static retval hook_##name_(__VA_ARGS__);                                                                                                    \
    };                                                                                                                                              \
    HOOK_AUTO_INSTALL(name_);                                                                                                                       \
    retval Hook_##name_::hook_##name_(__VA_ARGS__)

#define MAKE_AUTO_HOOK_ORIG_MATCH(name_, mPtr, retval, ...)                                                                                         \
    struct Hook_##name_ {                                                                                                                           \
        using funcType = retval (*)(__VA_ARGS__);                                                                                                   \
        static_assert(std::is_same_v<funcType, ::Hooking::InternalMethodCheck<decltype(mPtr)>::funcType>, "Hook method signature does not match!"); \
        constexpr static const char* name() { return #name_; }                                                                                      \
        static const MethodInfo* getInfo() { return ::il2cpp_utils::il2cpp_type_check::MetadataGetter<mPtr>::methodInfo(); }                        \
        static funcType* trampoline() { return &name_; }                                                                                            \
        static inline retval (*name_)(__VA_ARGS__) = nullptr;                                                                                       \
        static funcType hook() { return &::Hooking::HookCatchWrapper<&hook_##name_, funcType>::wrapper; }                                           \
        static retval hook_##name_(__VA_ARGS__);                                                                                                    \
    };                                                                                                                                              \
    HOOK_AUTO_INSTALL_ORIG(name_);                                                                                                                  \
    retval Hook_##name_::hook_##name_(__VA_ARGS__)
