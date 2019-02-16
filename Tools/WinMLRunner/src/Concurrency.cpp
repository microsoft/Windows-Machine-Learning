#include "Windows.h"
#include "common.h"
#include <iostream>
#include <thread>

using namespace winrt;
using namespace winrt::Windows::AI::MachineLearning;

void load_model(const std::wstring &path, bool print_info)
{
  if (print_info)
  {
    std::wcout << L"Begin loading a model in thread "
               << std::this_thread::get_id() << std::endl;
  }
  auto model = LearningModel::LoadFromFilePath(path);
  if (print_info)
  {
    std::wcout << L"End loading a model in thread "
               << std::this_thread::get_id() << std::endl;
  }
}

void ConcurrentLoadModel(const std::wstring &path, unsigned num_threads,
                         unsigned interval_milliseconds, bool print_info)
{
  std::vector<std::thread> threads;
  for (unsigned i = 0; i < num_threads; i++)
  {
      threads.emplace_back(std::thread(load_model, std::ref(path), print_info));
      Sleep(interval_milliseconds);
  }
  std::for_each(threads.begin(), threads.end(), [](std::thread &th) { th.join(); });
}
