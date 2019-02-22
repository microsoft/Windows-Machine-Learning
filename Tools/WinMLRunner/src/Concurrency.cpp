#include "Windows.h"
#include "common.h"
#include <iostream>
#include <thread>
#include <regex>

using namespace winrt;
using namespace winrt::Windows::AI::MachineLearning;

void load_model(const std::wstring &path, bool print_info)
{
  if (print_info)
  {
    std::wstringstream ss;
    ss <<  L"Begin loading a model " << path << L" in thread " << std::this_thread::get_id() << std::endl;
    std::wcout << ss.str();
  }
  auto model = LearningModel::LoadFromFilePath(path);
  if (print_info)
  {
    std::wstringstream ss;
    ss << L"End loading a model in thread " << std::this_thread::get_id() << std::endl;
    std::wcout << ss.str();
  }
}

void ConcurrentLoadModel(const std::vector<std::wstring> &paths, unsigned num_threads,
                         unsigned interval_milliseconds, bool print_info)
{
  std::vector<std::thread> threads;
  unsigned threads_size = paths.size() > num_threads ? paths.size() : num_threads;
  for (unsigned i = 0; i < threads_size; i++)
  {
      threads.emplace_back(std::thread(load_model, std::ref(paths[i % paths.size()]), print_info));
      Sleep(interval_milliseconds);
  }
  std::for_each(threads.begin(), threads.end(), [](std::thread &th) { th.join(); });
}
