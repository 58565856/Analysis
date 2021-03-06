#include <string>
#include <cstdlib>
#include <sys/system_properties.h>
#include <SubstrateHook.h>
#include <memory>
#include <dlfcn.h>
#include <pthread.h>
#include "jd/native-lib.h"
#include "SymbolFinder.h"
#include "utils/Util.h"
#include "jd/native_bridge.h"
#include "lua.hpp"

//#include <execinfo.h>

static bool firstLoad = true;
static bool isFirst = true;

typedef int (*luaL_loadstring_def)(slua::lua_State *L, const char *s);

luaL_loadstring_def load_string;
//
slua::lua_State *g_luastate;





HOOK_DEF(int, luaL_loadstring, slua::lua_State *L, const char *s) {
    //std::string tmpstr(s);
    //faklog(tmpstr);
    write2file(s, "/sdcard/pubg.lua");
    return orig_luaL_loadstring(L, s);
}

void x86_spec() {
    char *error;
    void *handle = dlopen("/data/data/me.ele/lib/libtnet-3.1.14.so", RTLD_LAZY);
    if (!handle) {
        LOGD("dlopen: -->%s \n", dlerror());
        //exit(EXIT_FAILURE);
    } else {
        dlerror();    /* Clear any existing error */
        LOGD("load UE4 succ!! STEP into next stage!");
    }

    load_string = (luaL_loadstring_def) dlsym(handle, "luaL_loadstring");
    if ((error = dlerror()) != nullptr) {
        LOGD("dlsym: -->%s\n", error);
        //exit(EXIT_FAILURE);
    } else {
        //dlerror();
        LOGD("發現LUA load buffer !! STEP into next stage! 繼續hook it !");
        MSHookFunction(reinterpret_cast<void *>(load_string),
                       (void *) new_luaL_loadstring,
                       (void **) &orig_luaL_loadstring);
    }

    dlerror();
}


void hookX86(void *handle) {
    LOGV("FUCK HOUDINI ");
    if (handle != nullptr) {
        auto cbks = (android::NativeBridgeCallbacks *) dlsym(handle, "NativeBridgeItf");
        //MSHookFunction(&(cbks->getTrampoline), (void *) new_getTrampoline,
        //             (void **) &orig_getTrampoline);
        if (cbks != nullptr) {
            LOGV("我日可以version : %d ,: %p ", cbks->version, cbks->getTrampoline);
        }
    }
}

void obtainClassName(JNIEnv *jniEnv, jclass clazz) {
    jclass clsClazz = jniEnv->GetObjectClass(clazz);
    if (clsClazz == nullptr)
        LOGD("ERROR!");
    jmethodID methodId = jniEnv->GetMethodID(clsClazz, "getName", "()Ljava/lang/String;");
    if (methodId == nullptr)
        LOGD("ERROR!");
    auto className = (jstring) jniEnv->CallObjectMethod(clazz, methodId);
    if (className == nullptr)
        LOGD("ERROR!");
    jint strlen = jniEnv->GetStringUTFLength(className);
    if (strlen == 0) LOGD("ERROR!");
    std::unique_ptr<char[]> str(new char[strlen + 1]);
    jniEnv->GetStringUTFRegion(className, 0, strlen, str.get());
    jniEnv->DeleteLocalRef(clsClazz);
    jniEnv->DeleteLocalRef(className);
    LOGD("Calling class is: %s", str.get());
}

HOOK_DEF(void*, do_dlopen_V23, const char *name, int flags, const void *extinfo
) {
    void *ret = orig_do_dlopen_V23(name, flags, extinfo);
    onSoLoaded(name, ret);
    ////note: 能返回非空地址的是x86 库的地址，nullptr的话是arm架构的代码,需要翻译
    LOGD("do_dlopen : %s, return : %p.", name, ret);
    return ret;
}

HOOK_DEF(void*, RegisterNatives, JNIEnv *env, jclass clazz, const JNINativeMethod *methods,
         jint nMethods) {
    LOGD("env:%p,class:%p,methods:%p,num_method:%d", env, clazz, methods, nMethods);
    ////反射name
    //get_clazz_name(env, clazz);
    obtainClassName(env, clazz);
    /////開始反射java類加載
    for (int i = 0; i < nMethods; i++) {
        LOGD("NAME:%s,sign:%s,addr:%p", methods[i].name, methods[i].signature, methods[i].fnPtr);
    }
    //call orig func
    void *ret = orig_RegisterNatives(env, clazz, methods, nMethods);
    LOGD("ORIG RET:%d", (int) ret);
    return ret;
}

HOOK_DEF(void*, FindClass, JNIEnv *env, const char *classname) {
    LOGD("FindClass name: %s", classname);
    return orig_FindClass(env, classname);
}

HOOK_DEF(void*, JNI_OnLoad, void *javaVM) {
    auto *vm = reinterpret_cast<JavaVM *>(javaVM);
    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    assert(env != nullptr);
    const struct JNINativeInterface *nativeInterface = env->functions;
    //nativeInterface->RegisterNatives(jclass clazz, const JNINativeMethod* methods, jint nMethods);
    if (nativeInterface->RegisterNatives != nullptr && firstLoad && nativeInterface->FindClass !=
                                                                    nullptr) {
        MSHookFunction(reinterpret_cast<void *>(nativeInterface->RegisterNatives),
                       (void *) new_RegisterNatives,
                       (void **) &orig_RegisterNatives);
        MSHookFunction(reinterpret_cast<void *>(nativeInterface->FindClass),
                       (void *) new_FindClass,
                       (void **) &orig_FindClass);
        LOGD("Hook RegisterNatives & Findclass succ!!!");
        firstLoad = false;
    }
    LOGD("RegisterNatives addr: %p", nativeInterface->RegisterNatives);

    return orig_JNI_OnLoad(javaVM);
}


void onSoLoaded(const char *name, void *handle) {
    void *symbol = nullptr;
#ifdef __i386__
    if (strstr(name, "libhoudini") != nullptr) {
        hookX86(handle);
//        if (findSymbol("JNI_OnLoad", "libnb.so", (unsigned long *) &symbol) == 0) {
//            LOGD("FIND JNI_ONLOAD FUCK IT ：%p", symbol);
//            MSHookFunction(symbol, (void *) new_JNI_OnLoad, (void **) &orig_JNI_OnLoad);
//        }
    }
#endif
    if (strstr(name, "jdbitmapkit") != nullptr) {
        if (findSymbol("JNI_OnLoad", "libjdbitmapkit.so", (unsigned long *) &symbol) == 0 && isFirst) {
            LOGD("FIND JNI_ONLOAD FUCK IT ：%p", symbol);
            MSHookFunction(symbol, (void *) new_JNI_OnLoad, (void **) &orig_JNI_OnLoad);
        }
    }
}

int findSymbol(const char *name, const char *libn,
               unsigned long *addr) {
    return find_name(getpid(), name, libn, addr);
}

HOOK_DEF(void*, do_dlopen_V28, const char *name, int flags, const void *extinfo,
         void *caller_addr) {
    LOGD("dlopen_28: %s %d  %p  %p", name, flags, extinfo, caller_addr);
    void *ret = orig_do_dlopen_V28(name, flags, extinfo, caller_addr);
    onSoLoaded(name, ret);
    return orig_do_dlopen_V28(name,flags,extinfo,caller_addr);
}

void hook_dlopen(int api_level) {
    void *symbol = nullptr;
    if (api_level <= ANDROID_M) {
        ////note: x86和arm 的linker的dlopen符号一致
        ////处理细节不同。请注意
        if (findSymbol("__dl__Z9do_dlopenPKciPK17android_dlextinfo", "linker",
                       (unsigned long *) &symbol) == 0) {
            MSHookFunction(symbol, (void *) new_do_dlopen_V23,
                           (void **) &orig_do_dlopen_V23);
        }
    } else if (api_level >= ANDROID_P) {
        //// 这段代码没有鸡巴用。所以以后用吧的，我操；
        LOGV("ANDROID 9.0");
        if (findSymbol("__dl__Z9do_dlopenPKciPK17android_dlextinfoPKv", "linker",
                       (unsigned long *) &symbol) == 0) {
            MSHookFunction(symbol, (void *) new_do_dlopen_V28, (void **) &orig_do_dlopen_V28);
        }
    } else {
        LOGD("NOT suport OS-api:%d", api_level);
    }
}

void hook_main() {
    char sdk_ver_str[PATH_MAX] = "0";
    __system_property_get("ro.build.version.sdk", sdk_ver_str);
    int api_level = atoi(sdk_ver_str);
    hook_dlopen(api_level);
}

void __attribute__((constructor)) init_so() {
    LOGD("into so hook module!!!");
    char abi[PATH_MAX] = "";
    __system_property_get("ro.product.cpu.abi", abi);
    LOGD("SYTEM-API: %s", abi);
    if (strstr(abi, "x86") != nullptr) {
        x86_spec();
    } else {
        hook_main();
    }
}
