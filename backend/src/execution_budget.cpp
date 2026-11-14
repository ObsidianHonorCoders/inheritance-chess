#include "execution_budget.h"
#include <stdexcept>
#include <utility>

namespace Chess::Backend
{
  thread_local ExecutionBudget* ExecutionBudget::current_ = nullptr;

  const char* ExecutionDeadlineExceeded::what() const noexcept
  {
    return "The request execution deadline was exceeded.";
  }

  ExecutionBudget::ExecutionBudget(std::chrono::milliseconds duration, Clock clock)
    : previous_(current_)
  {
    if (duration < std::chrono::milliseconds(1) || duration > std::chrono::milliseconds(60000))
    {
      throw std::invalid_argument("Execution budget must be between 1 and 60000 milliseconds.");
    }
    clock_ = previous_ ? previous_->clock_ : std::move(clock);
    if (!clock_)
    {
      clock_ = [] { return std::chrono::steady_clock::now(); };
    }
    deadline_ = clock_() + duration;
    if (previous_ && previous_->deadline_ < deadline_)
    {
      deadline_ = previous_->deadline_;
    }
    current_ = this;
  }

  ExecutionBudget::~ExecutionBudget() noexcept
  {
    current_ = previous_;
  }

  bool ExecutionBudget::expired()
  {
    return current_ && current_->clock_() >= current_->deadline_;
  }

  void ExecutionBudget::checkpoint()
  {
    if (expired())
    {
      throw ExecutionDeadlineExceeded{};
    }
  }
}