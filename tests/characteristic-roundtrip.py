#!/usr/bin/env python3
"""Run the actual two hook bodies with a small native-contract/registry harness.

This host test verifies dispatch and output writes, not Android hook installation.
The unpatched control reproduces the native enum-101 -> Standard data-loss path.
"""
from pathlib import Path
import json
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/Hooks/CharacteristicsHooks.cpp").read_text()
bodies = source[source.index("MAKE_AUTO_HOOK_MATCH(BeatmapCharacteristicExtensions_SerializedName,"):]
prefix = r'''
#include <cassert>
#include <optional>
#include <string>
#include <unordered_map>
#include <iostream>
namespace GlobalNamespace {
struct BeatmapCharacteristic { int value__; explicit BeatmapCharacteristic(int x=0):value__(x){} };
}
using GlobalNamespace::BeatmapCharacteristic;
struct StringW {
    std::string value; bool present;
    StringW(std::string s):value(std::move(s)),present(true){}
    StringW(char const* s):value(s?s:""),present(s!=nullptr){}
    explicit operator bool() const { return present; }
    operator std::string() const { return value; }
};
template<class T> struct by_ref {
    T* p; explicit by_ref(T& value):p(&value){}
    T& operator*() const { return *p; }
    by_ref& operator=(T& value) { p=&value; return *this; }
};
namespace SongCore::API::Characteristics {
struct Info { std::string serializedName; int sortingOrder; };
std::unordered_map<std::string,Info> registry;
std::optional<Info> GetCharacteristic(BeatmapCharacteristic c) {
    for (auto const& [name,info]:registry) if(info.sortingOrder==c.value__)return info;
    return std::nullopt;
}
std::optional<Info> GetCharacteristicBySerializedName(std::string const& name) {
    auto it=registry.find(name); if(it!=registry.end())return it->second; return std::nullopt;
}
}
std::string const builtin[]{"Standard","OneSaber","Legacy","NoArrows","360Degree","90Degree"};
int nativeSerializeCalls=0,nativeParseCalls=0;
StringW BeatmapCharacteristicExtensions_SerializedName(BeatmapCharacteristic c) {
    ++nativeSerializeCalls; return builtin[c.value__>=0&&c.value__<6?c.value__:0];
}
bool BeatmapCharacteristicExtensions_FromSerializedName(StringW s,by_ref<BeatmapCharacteristic> out) {
    ++nativeParseCalls; *out=BeatmapCharacteristic(0);
    for(int i=0;i<6;++i)if(s&&s.value==builtin[i]){*out=BeatmapCharacteristic(i);return true;}
    return false;
}
#define MAKE_AUTO_HOOK_MATCH(name, method, result, ...) result name##_hook(__VA_ARGS__)
'''
suffix = r'''
int main() {
    using namespace SongCore::API::Characteristics;
    registry.emplace("Lawless",Info{"Lawless",101});
    registry.emplace("Lightshow",Info{"Lightshow",100});
    registry.emplace("MissingCharacteristic",Info{"MissingCharacteristic",1000});
    int cases=0;
    for(int i=0;i<6;++i) {
        auto s=BeatmapCharacteristicExtensions_SerializedName_hook(BeatmapCharacteristic(i));
        assert(s.value==builtin[i]);
        BeatmapCharacteristic out(-99);
        assert(BeatmapCharacteristicExtensions_FromSerializedName_hook(s,by_ref(out))&&out.value__==i);
        ++cases;
    }
    assert(nativeSerializeCalls==6&&nativeParseCalls==6);
    for(auto const& [name,info]:registry) {
        BeatmapCharacteristic out(-99);
        auto s=BeatmapCharacteristicExtensions_SerializedName_hook(BeatmapCharacteristic(info.sortingOrder));
        assert(s.value==name);
        assert(BeatmapCharacteristicExtensions_FromSerializedName_hook(s,by_ref(out))&&out.value__==info.sortingOrder);
        ++cases;
    }
    // An unknown and null name keep the native false result and zero output.
    for(auto name:{StringW("unregistered"),StringW(nullptr)}) {
        BeatmapCharacteristic out(123);
        assert(!BeatmapCharacteristicExtensions_FromSerializedName_hook(name,by_ref(out))&&out.value__==0);
        ++cases;
    }
    assert(BeatmapCharacteristicExtensions_SerializedName_hook(BeatmapCharacteristic(-1)).value=="Standard");
    assert(BeatmapCharacteristicExtensions_SerializedName_hook(BeatmapCharacteristic(999)).value=="Standard");
    // The registry remains live; late registration and removal are not cached.
    registry.emplace("Neon",Info{"Neon",102});
    BeatmapCharacteristic out(-1);
    assert(BeatmapCharacteristicExtensions_FromSerializedName_hook(StringW("Neon"),by_ref(out))&&out.value__==102);
    registry.erase("Neon");
    assert(!BeatmapCharacteristicExtensions_FromSerializedName_hook(StringW("Neon"),by_ref(out))&&out.value__==0);
    cases+=4;
    // Preserve all six builtins even if a custom registration reuses a builtin name.
    registry.emplace("Standard",Info{"Standard",999});
    assert(BeatmapCharacteristicExtensions_FromSerializedName_hook(StringW("Standard"),by_ref(out))&&out.value__==0);
    // Negative control: actual old native dispatch loses the custom key on save.
    auto old=BeatmapCharacteristicExtensions_SerializedName(BeatmapCharacteristic(101));
    assert(old.value=="Standard");
    assert(BeatmapCharacteristicExtensions_FromSerializedName(old,by_ref(out))&&out.value__!=101);
    std::cout<<"{\"passed\":true,\"dispatch_cases\":"<<cases+1<<",\"original_negative_control_reproduced\":true}\n";
}
'''
with tempfile.TemporaryDirectory(prefix="songcore-characteristic-") as tmp:
    cpp = Path(tmp)/"test.cpp"
    exe = Path(tmp)/"test"
    cpp.write_text(prefix+bodies+suffix)
    subprocess.run([os.environ.get("CXX", "c++"),"-std=c++23","-O1",str(cpp),"-o",str(exe)],check=True)
    result=json.loads(subprocess.check_output([str(exe)],text=True))
    result["scope"]="Actual extracted production hook bodies; mocked managed strings, by_ref and registry. This host harness does not test Android hook installation or game-version compatibility."
    print(json.dumps(result,indent=2))
