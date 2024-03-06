#pragma once

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include <chrono>
#include <thread>

namespace SongCore {
    template<typename Ret, typename T>
    requires(std::is_invocable_r_v<Ret, T>)
    static void task_func(System::Threading::Tasks::Task_1<Ret>* task, T func) {
        task->TrySetResult(std::invoke(func));
    }

    template<typename Ret, typename T>
    requires(std::is_invocable_r_v<Ret, T, System::Threading::CancellationToken>)
    static void task_cancel_func(System::Threading::Tasks::Task_1<Ret>* task, T func, System::Threading::CancellationToken cancelToken) {
        using namespace std::chrono_literals;
        // start func as an std::future
        auto fut = il2cpp_utils::il2cpp_async(func, cancelToken);
        // await the future
        while (fut.wait_for(0ns) != std::future_status::ready && !cancelToken.IsCancellationRequested) std::this_thread::yield();

        // if cancellation wasn't requested, set result, else set canceled
        if (!cancelToken.IsCancellationRequested) {
            task->TrySetResult(fut.get());
        } else {
            task->TrySetCanceled(cancelToken);
        }
    }

    template<typename Ret, typename T>
    requires(!std::is_same_v<Ret, void> && std::is_invocable_r_v<Ret, T>)
    static System::Threading::Tasks::Task_1<Ret>* StartTask(T func) {
        auto t = System::Threading::Tasks::Task_1<Ret>::New_ctor();
        il2cpp_utils::il2cpp_aware_thread(&task_func<Ret, T>, t, func).detach();
        return t;
    }

    template<typename Ret, typename T>
    requires(!std::is_same_v<Ret, void> && std::is_invocable_r_v<Ret, T, System::Threading::CancellationToken>)
    static System::Threading::Tasks::Task_1<Ret>* StartTask(T func, System::Threading::CancellationToken cancelToken) {
        auto t = System::Threading::Tasks::Task_1<Ret>::New_ctor();
        il2cpp_utils::il2cpp_aware_thread(&task_cancel_func<Ret, T>, t, func, cancelToken).detach();
        return t;
    }
}
