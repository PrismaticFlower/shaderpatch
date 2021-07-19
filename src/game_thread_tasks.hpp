#pragma once

#include <concepts>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <vector>

namespace sp {

template<typename T>
class Game_thread_task_result {
public:
   Game_thread_task_result() = default;

   explicit Game_thread_task_result(std::future<T> future)
      : _future{std::move(future)}
   {
   }

   bool ready() const noexcept
   {
      return _future.valid() && (_future.wait_for(std::chrono::seconds{0}) ==
                                 std::future_status::ready);
   }

   auto get() noexcept -> T
   {
      if (!ready()) std::terminate();

      return _future.get();
   }

private:
   std::future<T> _future;
};

class Game_thread_tasks {
public:
   template<std::invocable Func>
   [[nodiscard]] auto schedule(Func&& func) noexcept
      -> Game_thread_task_result<std::invoke_result_t<Func>>
   {
      using Return = std::invoke_result_t<Func>;

      std::promise<Return> promise{};
      Game_thread_task_result result{promise.get_future()};

      std::scoped_lock lock{_tasks_mutex};

      _tasks.emplace_back(std::make_unique<Task<Return>>(std::move(promise),
                                                         std::forward<Func>(func)));

      return result;
   }

   void execute_all() noexcept
   {
      std::scoped_lock lock{_tasks_mutex};

      for (auto& task : _tasks) task->execute();

      _tasks.clear();
   }

private:
   struct Task_base {
      virtual ~Task_base() = default;

      virtual void execute() noexcept = 0;
   };

   template<typename T>
   struct Task : Task_base {
      std::promise<T> promise;
      std::function<T()> function;

      template<std::invocable Func>
      Task(std::promise<T> promise, Func&& func)
         : promise{std::move(promise)}, function{std::forward<Func>(func)}
      {
      }

      void execute() noexcept override
      {
         promise.set_value(function());
      }
   };

   std::mutex _tasks_mutex;
   std::vector<std::unique_ptr<Task_base>> _tasks;
};
}
