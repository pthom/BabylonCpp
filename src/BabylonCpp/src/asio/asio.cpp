#include <babylon/asio/asio.h>

#include <babylon/asio/internal/file_loader_sync.h>
#include <babylon/asio/internal/future_utils.h>
#include <babylon/core/filesystem.h>
#include <babylon/core/string.h>
#include <babylon/asio/internal/sync_callback_runner.h>

#if defined(__linux__) or defined(__APPLE__)
#define CAN_NAME_THREAD
#endif

#ifdef CAN_NAME_THREAD
  #include <pthread.h>
  #ifdef __APPLE__
    #define THIS_THREAD_SET_NAME(name)  pthread_setname_np(name)
  #elif defined(__linux__)
    #define THIS_THREAD_SET_NAME(name) pthread_setname_np(pthread_self(), name)
  #endif
#endif


#include <deque>
#include <future>
#include <atomic>
#include <chrono>

namespace BABYLON {
namespace asio {

using namespace sync_io_impl;

using OnSuccessFunctionArrayBuffer            = std::function<void(const ArrayBuffer& data)>;

using FutureArrayBufferOrErrorMessage = std::future<ArrayBufferOrErrorMessage>;


struct FutureAndCallbacks {
  OnSuccessFunctionArrayBuffer onSuccessFunctionArrayBuffer;
  OnErrorFunction onErrorFunction;
  FutureArrayBufferOrErrorMessage futureDataTypeOrErrorMessage;

  std::optional<VoidCallback> shallCallNextCallback()
  {
    auto status = get_future_running_status(futureDataTypeOrErrorMessage);
    if (status != future_running_status::future_ready)
      return std::nullopt;

    ArrayBufferOrErrorMessage v = futureDataTypeOrErrorMessage.get();

    VoidCallback nextCallback = EmptyVoidCallback;
    if (std::holds_alternative<ErrorMessage>(v))
    {
      if (onErrorFunction)
      {
        const auto& errorMessage = std::get<ErrorMessage>(v);

        auto onErrorFunctionCopy = this->onErrorFunction;
        nextCallback = [errorMessage, onErrorFunctionCopy]() {
          onErrorFunctionCopy(errorMessage.errorMessage);
        };
      }
    }

    if (std::holds_alternative<ArrayBuffer>(v))
    {
      if (onSuccessFunctionArrayBuffer)
      {
        const auto& data = std::get<ArrayBuffer>(v);

        auto onSuccessFunctionCopy = this->onSuccessFunctionArrayBuffer;
        nextCallback = [onSuccessFunctionCopy, data]() {
          onSuccessFunctionCopy(data);
        };
      }
    }
    return nextCallback;
  }

};


class AsyncLoadService {
private:
  AsyncLoadService()
  {
    mStopRequested       = false;
    mHasRunningIOTasks       = false;
    mCheckIOCompletion_Async = really_async([this]() { this->CheckIOCompletion_AsyncProc(); });
  }
  ~AsyncLoadService()
  {
    mStopRequested = true;
  }

  void CheckTasksStatus()
  {
    std::lock_guard<std::mutex> guard(mMutexRunningIOTasks);
    std::vector<FutureAndCallbacks> stillRunningTasks;
    for (auto& runningTask : mRunningIOTasks)
    {
      std::optional<VoidCallback> nextCallback = runningTask.shallCallNextCallback();
      if (nextCallback.has_value())
        sync_callback_runner::PushCallback(std::move(nextCallback.value()));
      else
        stillRunningTasks.emplace_back(std::move(runningTask));
    }
    mRunningIOTasks  = std::move(stillRunningTasks);
    mHasRunningIOTasks = !mRunningIOTasks.empty();
  }

  void CheckIOCompletion_AsyncProc() // This will be called in a parallel thread
  {
    using namespace std::literals;
#ifdef CAN_NAME_THREAD
    THIS_THREAD_SET_NAME("asio: CheckIOCompletion");
#endif
    while (!mStopRequested) {
      CheckTasksStatus();
      std::this_thread::sleep_for(15ms);
    }
  }

public:
  void LoadData(
    const SyncLoaderFunction& syncLoader,
    const OnSuccessFunctionArrayBuffer & onSuccessFunctionArrayBuffer,
    const OnErrorFunction& onErrorFunction
  )
  {
    std::lock_guard<std::mutex> guard(mMutexRunningIOTasks);
    mHasRunningIOTasks = true;
    FutureArrayBufferOrErrorMessage futureData = really_async(syncLoader);
    FutureAndCallbacks payload{onSuccessFunctionArrayBuffer, onErrorFunction, std::move(futureData)};

    mRunningIOTasks.emplace_back(std::move(payload));
  }

  static AsyncLoadService& Instance()
  {
    static AsyncLoadService instance;
    return instance;
  }

  void WaitIoCompletion_Sync()
  {
    using namespace std::literals;
    while(mHasRunningIOTasks)
      std::this_thread::sleep_for(50ms);
  }

  bool HasRunningIOTasks()
  {
    return mHasRunningIOTasks;
  }

private:
  std::vector<FutureAndCallbacks> mRunningIOTasks;
  std::atomic<bool> mHasRunningIOTasks;
  std::mutex mMutexRunningIOTasks;

  std::future<void> mCheckIOCompletion_Async;
  std::atomic<bool> mStopRequested;

};

static std::string ArrayBufferToString(const ArrayBuffer & dataUint8)
{
  std::string dataString;
  dataString.resize(dataUint8.size());
  for (size_t i = 0; i < dataUint8.size(); ++i)
    dataString[i] = static_cast<char>(dataUint8[i]);
  dataString = BABYLON::String::replace(dataString, "\r\n", "\n");
  return dataString;
}

void LoadFileAsync_Text(const std::string& filename,
                       const OnSuccessFunction<std::string>& onSuccessFunction,
                       const OnErrorFunction& onErrorFunction,
                       const OnProgressFunction& onProgressFunction
                       )
{
  auto & service = AsyncLoadService::Instance();
  auto syncLoader = [filename, onProgressFunction]() {
#ifdef CAN_NAME_THREAD
    THIS_THREAD_SET_NAME("asio: LoadFileSync_Text");
#endif
    return LoadFileSync_Binary(filename, onProgressFunction);
  };
  auto onSuccessFunctionArrayBuffer = [onSuccessFunction](const ArrayBuffer & dataUint8) {
    onSuccessFunction(ArrayBufferToString(dataUint8));
  };
  service.LoadData(syncLoader, onSuccessFunctionArrayBuffer, onErrorFunction);
}

void LoadFileAsync_Binary(
  const std::string& filename,
  const OnSuccessFunction<ArrayBuffer>& onSuccessFunction,
  const OnErrorFunction& onErrorFunction,
  const OnProgressFunction& onProgressFunction
  )
{
  auto & service = AsyncLoadService::Instance();
  auto syncLoader = [filename, onProgressFunction]() {
#ifdef CAN_NAME_THREAD
    THIS_THREAD_SET_NAME("asio: LoadFileSync_Binary");
#endif

    return LoadFileSync_Binary(filename, onProgressFunction);
  };
  service.LoadData(syncLoader, onSuccessFunction, onErrorFunction);
}

void LoadUrlAsync_Text(
  const std::string& url,
  const std::function<void(const std::string& data)>& onSuccessFunction,
  const OnErrorFunction& onErrorFunction,
  const OnProgressFunction& onProgressFunction
)
{
  if (!String::startsWith(url, "file:/"))
    throw std::runtime_error("LoadUrlAsync_Text only support files for the moment");
  std::string filename = url.substr(6);
  LoadFileAsync_Text(filename, onSuccessFunction, onErrorFunction, onProgressFunction);
}

void LoadUrlAsync_Binary(
  const std::string& url,
  const std::function<void(const ArrayBuffer& data)>& onSuccessFunction,
  const OnErrorFunction& onErrorFunction,
  const OnProgressFunction& onProgressFunction
)
{
  if (!String::startsWith(url, "file:/"))
    throw std::runtime_error("LoadUrlAsync_Text only support files for the moment");
  std::string filename = url.substr(6);
  LoadFileAsync_Binary(filename, onSuccessFunction, onErrorFunction, onProgressFunction);
}

// Call this in the app's main loop: it will run the callbacks synchronously
// after the io completion
void HeartBeat_Sync()
{
  sync_callback_runner::HeartBeat();
}


void Service_WaitAll_Sync()
{
  auto & service = AsyncLoadService::Instance();
  service.WaitIoCompletion_Sync();
  sync_callback_runner::CallAllPendingCallbacks();
}

bool HasRemainingTasks()
{
  auto & service = AsyncLoadService::Instance();
  return    service.HasRunningIOTasks()
         || sync_callback_runner::HasRemainingCallbacks();
}

} // namespace asio
} // namespace BABYLON