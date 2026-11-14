#ifndef ICHESS_BACKEND_EXECUTION_BUDGET
#define ICHESS_BACKEND_EXECUTION_BUDGET

#include <chrono>
#include <exception>
#include <functional>

namespace Chess::Backend
{
  class ExecutionDeadlineExceeded final : public std::exception
  {
    public:
      const char* what() const noexcept override;
  };

  class ExecutionBudget final
  {
    public:
      using Clock = std::function<std::chrono::steady_clock::time_point()>;

      explicit ExecutionBudget(std::chrono::milliseconds duration, Clock clock = {});
      ~ExecutionBudget() noexcept;

      ExecutionBudget(const ExecutionBudget&) = delete;
      ExecutionBudget& operator=(const ExecutionBudget&) = delete;
      ExecutionBudget(ExecutionBudget&&) = delete;
      ExecutionBudget& operator=(ExecutionBudget&&) = delete;

      static bool expired();
      static void checkpoint();

    private:
      static thread_local ExecutionBudget* current_;
      ExecutionBudget* previous_;
      Clock clock_;
      std::chrono::steady_clock::time_point deadline_;
  };
}

#endif