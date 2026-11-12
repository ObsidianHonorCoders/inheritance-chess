#ifndef ICHESS_BACKEND_LOCAL_ACCOUNT_STORE
#define ICHESS_BACKEND_LOCAL_ACCOUNT_STORE

#include "api_types.h"
#include <memory>
#include <optional>
#include <string_view>

namespace Chess::Backend
{
  enum class AccountCreateResult { Created, AlreadyExists, Unavailable };

  class ILocalAccountStore
  {
    public:
      virtual ~ILocalAccountStore() = default;
      virtual bool is_ready() const noexcept = 0;
      virtual AccountCreateResult create_account(std::string_view username,
                                                 std::string_view password) noexcept = 0;
      virtual std::optional<Principal> authenticate(std::string_view username,
                                                    std::string_view password) noexcept = 0;
  };

  class SqliteLocalAccountStore final : public ILocalAccountStore
  {
    public:
      explicit SqliteLocalAccountStore(const std::string& path);
      ~SqliteLocalAccountStore() override;

      SqliteLocalAccountStore(const SqliteLocalAccountStore&) = delete;
      SqliteLocalAccountStore& operator=(const SqliteLocalAccountStore&) = delete;

      bool is_ready() const noexcept override;
      AccountCreateResult create_account(std::string_view username,
                                         std::string_view password) noexcept override;
      std::optional<Principal> authenticate(std::string_view username,
                                            std::string_view password) noexcept override;

    private:
      struct State;
      std::unique_ptr<State> state_;
  };
}

#endif