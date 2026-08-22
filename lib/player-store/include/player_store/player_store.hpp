#ifndef PRADING_PLAYER_STORE_HPP
#define PRADING_PLAYER_STORE_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace PlayerStore {

enum class ErrorCode {
    invalid_username,
    invalid_record,
    authentication_failed,
    not_found,
    sqlite,
};

struct Error {
    ErrorCode code;
    std::string message;
};

template <typename Value>
class Result {
public:
    Result(Value value) : value_(std::move(value)) {}
    Result(Error error) : value_(std::move(error)) {}

    bool has_value() const { return std::holds_alternative<Value>(value_); }
    explicit operator bool() const { return has_value(); }
    Value& value() { return std::get<Value>(value_); }
    const Value& value() const { return std::get<Value>(value_); }
    const Error& error() const { return std::get<Error>(value_); }

private:
    std::variant<Value, Error> value_;
};

template <>
class Result<void> {
public:
    Result() = default;
    Result(Error error) : error_(std::move(error)) {}

    bool has_value() const { return !error_.has_value(); }
    explicit operator bool() const { return has_value(); }
    const Error& error() const { return *error_; }

private:
    std::optional<Error> error_;
};

struct Record {
    std::string username;
    std::string password_hash;
    std::chrono::system_clock::time_point last_login;
    bool has_logged_in = false;
    std::chrono::system_clock::time_point last_saved;
    std::string player_text;
    int player_version = 1;
};

using PasswordVerifier = std::function<bool(std::string_view password,
                                             std::string_view password_hash)>;

class Storage {
public:
    virtual ~Storage() = default;

    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    virtual Result<Record> restore(std::string_view username) const = 0;
    virtual Result<Record> login(std::string_view username,
                                 std::string_view password,
                                 const PasswordVerifier& verify) = 0;
    virtual Result<void> save(const Record& record) = 0;
    virtual Result<void> remove(std::string_view username) = 0;

protected:
    Storage() = default;
};

Result<std::unique_ptr<Storage>> open(const std::string& filename);
bool is_valid_username(std::string_view username);

} // namespace PlayerStore

#endif