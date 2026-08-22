#include "player_store/player_store.hpp"

#include <sqlite3.h>

#include <cctype>
#include <ctime>
#include <utility>

namespace PlayerStore {
namespace {

Error sqlite_error(sqlite3* database, const char* operation) {
    return {ErrorCode::sqlite, std::string(operation) + ": " + sqlite3_errmsg(database)};
}

Error make_error(ErrorCode code, std::string message) {
    return {code, std::move(message)};
}

class Statement {
public:
    Statement(sqlite3* database, const char* sql) {
        status_ = sqlite3_prepare_v2(database, sql, -1, &statement_, nullptr);
    }
    ~Statement() { sqlite3_finalize(statement_); }

    int status() const { return status_; }
    sqlite3_stmt* get() const { return statement_; }

private:
    sqlite3_stmt* statement_ = nullptr;
    int status_ = SQLITE_ERROR;
};

Result<Record> read_record(sqlite3* database, sqlite3_stmt* statement) {
    const int result = sqlite3_step(statement);
    if (result == SQLITE_DONE)
        return make_error(ErrorCode::not_found, "player not found");
    if (result != SQLITE_ROW)
        return sqlite_error(database, "reading player");

    Record record;
    record.username = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
    record.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
    if (sqlite3_column_type(statement, 2) != SQLITE_NULL) {
        record.last_login = std::chrono::system_clock::from_time_t(
            static_cast<time_t>(sqlite3_column_int64(statement, 2)));
        record.has_logged_in = true;
    }
    record.last_saved = std::chrono::system_clock::from_time_t(
        static_cast<time_t>(sqlite3_column_int64(statement, 3)));
    record.player_text = reinterpret_cast<const char*>(sqlite3_column_text(statement, 4));
    record.player_version = sqlite3_column_int(statement, 5);
    return record;
}

constexpr const char* record_columns =
    "username, password_hash, last_login, last_saved, player_text, player_version";

class SqliteStorage final : public Storage {
public:
    explicit SqliteStorage(sqlite3* database) : database_(database) {}
    ~SqliteStorage() override { sqlite3_close(database_); }

    Result<Record> restore(std::string_view username) const override;
    Result<Record> login(std::string_view username,
                         std::string_view password,
                         const PasswordVerifier& verify) override;
    Result<void> save(const Record& record) override;
    Result<void> remove(std::string_view username) override;

private:
    sqlite3* database_;
};

} // namespace

bool is_valid_username(std::string_view username) {
    if (username.empty() || username.size() > 32)
        return false;
    for (const unsigned char character : username) {
        if (!(std::isalnum(character) || character == '_' || character == '-'))
            return false;
    }
    return true;
}

Result<std::unique_ptr<Storage>> open(const std::string& filename) {
    sqlite3* database = nullptr;
    if (sqlite3_open_v2(filename.c_str(), &database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        Error result = sqlite_error(database, "opening player store");
        sqlite3_close(database);
        return result;
    }
    if (sqlite3_busy_timeout(database, 5000) != SQLITE_OK) {
        Error result = sqlite_error(database, "configuring player store");
        sqlite3_close(database);
        return result;
    }
    const char* schema =
        "CREATE TABLE IF NOT EXISTS players ("
        "username TEXT PRIMARY KEY, password_hash TEXT NOT NULL, last_login INTEGER, "
        "last_saved INTEGER NOT NULL, player_text TEXT NOT NULL, "
        "player_version INTEGER NOT NULL DEFAULT 1)";
    if (sqlite3_exec(database, schema, nullptr, nullptr, nullptr) != SQLITE_OK) {
        Error result = sqlite_error(database, "creating player store schema");
        sqlite3_close(database);
        return result;
    }
    return std::unique_ptr<Storage>(new SqliteStorage(database));
}

Result<Record> SqliteStorage::restore(std::string_view username) const {
    if (!is_valid_username(username))
        return make_error(ErrorCode::invalid_username, "invalid username");
    const std::string sql = "SELECT " + std::string(record_columns) + " FROM players WHERE username = ?1";
    Statement statement(database_, sql.c_str());
    if (statement.status() != SQLITE_OK)
        return sqlite_error(database_, "preparing restore");
    sqlite3_bind_text(statement.get(), 1, username.data(), static_cast<int>(username.size()), SQLITE_TRANSIENT);
    return read_record(database_, statement.get());
}

Result<Record> SqliteStorage::login(std::string_view username,
                                  std::string_view password,
                                  const PasswordVerifier& verify) {
    if (!is_valid_username(username))
        return make_error(ErrorCode::invalid_username, "invalid username");
    if (!verify)
        return make_error(ErrorCode::invalid_record, "password verifier is required");

    if (sqlite3_exec(database_, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK)
        return sqlite_error(database_, "starting login transaction");

    const std::string sql = "SELECT " + std::string(record_columns) + " FROM players WHERE username = ?1";
    Statement statement(database_, sql.c_str());
    if (statement.status() != SQLITE_OK) {
        sqlite3_exec(database_, "ROLLBACK", nullptr, nullptr, nullptr);
        return sqlite_error(database_, "preparing login");
    }
    sqlite3_bind_text(statement.get(), 1, username.data(), static_cast<int>(username.size()), SQLITE_TRANSIENT);
    auto result = read_record(database_, statement.get());
    if (!result) {
        sqlite3_exec(database_, "ROLLBACK", nullptr, nullptr, nullptr);
        if (result.error().code == ErrorCode::not_found)
            return make_error(ErrorCode::authentication_failed, "authentication failed");
        return result.error();
    }
    Record record = std::move(result.value());
    if (!verify(password, record.password_hash)) {
        sqlite3_exec(database_, "ROLLBACK", nullptr, nullptr, nullptr);
        return make_error(ErrorCode::authentication_failed, "authentication failed");
    }

    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    Statement update(database_, "UPDATE players SET last_login = ?1 WHERE username = ?2");
    if (update.status() != SQLITE_OK) {
        sqlite3_exec(database_, "ROLLBACK", nullptr, nullptr, nullptr);
        return sqlite_error(database_, "preparing login update");
    }
    sqlite3_bind_int64(update.get(), 1, static_cast<sqlite3_int64>(now));
    sqlite3_bind_text(update.get(), 2, username.data(), static_cast<int>(username.size()), SQLITE_TRANSIENT);
    if (sqlite3_step(update.get()) != SQLITE_DONE || sqlite3_exec(database_, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_exec(database_, "ROLLBACK", nullptr, nullptr, nullptr);
        return sqlite_error(database_, "committing login");
    }
    record.last_login = std::chrono::system_clock::from_time_t(now);
    record.has_logged_in = true;
    return record;
}

Result<void> SqliteStorage::save(const Record& record) {
    if (!is_valid_username(record.username))
        return make_error(ErrorCode::invalid_username, "invalid username");
    if (record.password_hash.empty() || record.player_version < 1)
        return make_error(ErrorCode::invalid_record, "invalid player record");

    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    Statement statement(database_,
        "INSERT INTO players (username, password_hash, last_login, last_saved, player_text, player_version) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6) "
        "ON CONFLICT(username) DO UPDATE SET password_hash = excluded.password_hash, "
        "last_login = excluded.last_login, last_saved = excluded.last_saved, "
        "player_text = excluded.player_text, player_version = excluded.player_version");
    if (statement.status() != SQLITE_OK)
        return sqlite_error(database_, "preparing save");
    sqlite3_bind_text(statement.get(), 1, record.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement.get(), 2, record.password_hash.c_str(), -1, SQLITE_TRANSIENT);
    if (record.has_logged_in)
        sqlite3_bind_int64(statement.get(), 3, static_cast<sqlite3_int64>(std::chrono::system_clock::to_time_t(record.last_login)));
    else
        sqlite3_bind_null(statement.get(), 3);
    sqlite3_bind_int64(statement.get(), 4, static_cast<sqlite3_int64>(now));
    sqlite3_bind_text(statement.get(), 5, record.player_text.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement.get(), 6, record.player_version);
    if (sqlite3_step(statement.get()) != SQLITE_DONE)
        return sqlite_error(database_, "saving player");
    return {};
}

Result<void> SqliteStorage::remove(std::string_view username) {
    if (!is_valid_username(username))
        return make_error(ErrorCode::invalid_username, "invalid username");
    Statement statement(database_, "DELETE FROM players WHERE username = ?1");
    if (statement.status() != SQLITE_OK)
        return sqlite_error(database_, "preparing remove");
    sqlite3_bind_text(statement.get(), 1, username.data(), static_cast<int>(username.size()), SQLITE_TRANSIENT);
    if (sqlite3_step(statement.get()) != SQLITE_DONE)
        return sqlite_error(database_, "removing player");
    return {};
}

} // namespace PlayerStore
