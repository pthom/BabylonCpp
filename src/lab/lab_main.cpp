int main() { return 0; }
/*
#include <stdio.h>
#include <string>
#include <map>
#include <chrono>
#include <thread>
#include <utility>
#include <future>
#include <iostream>
#include <functional>

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wformat-pedantic"
#endif
#include "promise.hpp"
#include "simple_task/simple_task.hpp"

using VoidCallback = std::function<void(void)>;

std::vector<std::future<void>> gAsyncProcs;

void myDelay(uint64_t time_ms, const VoidCallback& callback)
{
  auto proc = [=]() {
    try
    {
      std::cout << "myDelay, about to sleep for " << time_ms << "ms\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(time_ms));
      std::cout << "myDelay, done sleep\n";
      callback();
    }
    catch(...)
    {
      std::cout << "Ouch\n";
    }
  };
  gAsyncProcs.push_back(std::async(std::launch::async, proc));
  //proc();
}

promise::Defer myDelayDefer(Service &service, uint64_t time_ms) {
  return promise::newPromise([time_ms](promise::Defer d) {
    myDelay(time_ms,
            [d]() {
              std::cout << "myDelayDefer inside callback\n";
              d.resolve();
            });
  });
}



promise::Defer testTimer(Service &service) {

  return service.yield().then([&] {
    printf("immediate!\n");
    return service.delay(1000);
  }).then([&] {
    printf("timer after 1000 ms!\n");
    return service.delay(2000);
  }).then([] {
    printf("timer after 2000 ms!\n");
  }).fail([] {
    printf("timer cancelled!\n");
  });
}

promise::Defer testTruc(Service & service) {
  return service.delay(100).then([&] {
    printf("After delay 100 !\n");
    return myDelayDefer(service, 3000);
  });
}

int main() {

  Service service;

  testTimer(service);
  testTimer(service);
  testTimer(service);

//  testTruc(service);
//  testTruc(service);
//  testTruc(service);

  //myDelayDefer(service, 1000);
  //myDelayDefer(service, 1000);

//  service.yield().then([&service]{
//    myDelayDefer(service, 1000);
//  });


  service.run();

  for (auto & v: gAsyncProcs)
    v.get();
  return 0;
}
*/