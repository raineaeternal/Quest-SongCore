param($p1, $p2)
$NDKPath = $env:ANDROID_NDK_HOME

if ($p1 -and $p2)
{
    & X:\android-ndk-r22b\toolchains\llvm\prebuilt\windows-x86_64\bin\aarch64-linux-android-addr2line.exe -e .\build\debug\$p1 $p2
}
else
{
    if ($p1)
    {
        & X:\android-ndk-r22b\toolchains\llvm\prebuilt\windows-x86_64\bin\aarch64-linux-android-addr2line.exe -e .\build\debug\libsongcore.so $p1
    }
    else
    {
        echo give at least 1 argument
    }
}
