#ifndef ICHESS_BACKEND_LOCAL_ACCOUNT_STORE
#define ICHESS_BACKEND_LOCAL_ACCOUNT_STORE

#include "api_types.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

namespace Chess::Backend
{
  enum class AccountCreateResult { Created, AlreadyExists, Unavailable };
  enum class PasswordChangeResult { Changed, InvalidCurrentPassword, Unavailable };

  class ILocalAccountStore
  {
    public:
      virtual ~ILocalAccountStore() = default;
      virtual bool is_ready() const noexcept = 0;
      virtual AccountCreateResult create_account(std::string_view username,
                                                 std::string_view password) noexcept = 0;
      virtual std::optional<Principal> authenticate(std::string_view username,
                                                    std::string_view password) noexcept = 0;
        virtual PasswordChangeResult change_password(std::string_view username,
          std::string_view current_password, std::string_view new_password) noexcept = 0;
  };

  class SqliteLocalAccountStore final : public ILocalAccountStore
  {
    public:
      explicit SqliteLocalAccountStore(const std::string& path);
      ~SqliteLocalAccountStore() override;

      SqliteLocalAccountStore(const SqliteLocalAccountStore&) = delete;
      SqliteLocalAccountStore& operator=(const SqliteLocalAccountStore&) = delete;

      bool is_ready() const noexcept override;
      bool backup_to(const std::filesystem::path& destination) const noexcept;
      AccountCreateResult create_account(std::string_view username,
                                         std::string_view password) noexcept override;
      std::optional<Principal> authenticate(std::string_view username,
                                            std::string_view password) noexcept override;
        PasswordChangeResult change_password(std::string_view username,
          std::string_view current_password, std::string_view new_password) noexcept override;

    private:
      struct State;
      std::unique_ptr<State> state_;
  };
}

#endif