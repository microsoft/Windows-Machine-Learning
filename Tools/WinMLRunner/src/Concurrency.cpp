#include <iostream>
#include <thread>
#include <regex>

#include "Windows.h"
#include "common.h"
#include "ThreadPool.h"

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

  ThreadPool pool(num_threads);
  // Creating enough threads to load all the models specified
  // If there is more than enough threads, some threads will concurrently load same models
  size_t threads_size = paths.size() > num_threads ? paths.size() : num_threads;
  std::vector<std::future<void>> output_futures;
  for (size_t i = 0; i < threads_size; i++)
  {
      Sleep(interval_milliseconds);
      output_futures.push_back(pool.SubmitWork(load_model, std::ref(paths[i % paths.size()]), true));
  }
  // TODO: read output values from load_model
}
