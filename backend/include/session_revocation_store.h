#ifndef ICHESS_BACKEND_SESSION_REVOCATION_STORE
#define ICHESS_BACKEND_SESSION_REVOCATION_STORE

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace Chess::Backend
{
  class ISessionRevocationStore
  {
    public:
      virtual ~ISessionRevocationStore() = default;
      virtual bool is_revoked(std::string_view token, std::int64_t now) = 0;
      virtual void revoke(std::string_view token, std::int64_t expires) = 0;
  };

  class SqliteSessionRevocationStore final : public ISessionRevocationStore
  {
    public:
      explicit SqliteSessionRevocationStore(const std::string& path,
                                            std::size_t max_records = 65536);
      ~SqliteSessionRevocationStore() override;

      SqliteSessionRevocationStore(const SqliteSessionRevocationStore&) = delete;
      SqliteSessionRevocationStore& operator=(const SqliteSessionRevocationStore&) = delete;

      bool is_ready() const noexcept;
      bool is_revoked(std::string_view token, std::int64_t now) override;
      void revoke(std::string_view token, std::int64_t expires) override;

    private:
      struct State;
      std::unique_ptr<State> state_;
  };
}

#endif