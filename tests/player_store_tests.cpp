#include "player_store/player_store.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace {

class PlayerStoreTest : public testing::Test {
protected:
    void SetUp() override {
        path = std::filesystem::temp_directory_path() / "prading-player-store-test.sqlite";
        std::filesystem::remove(path);
    }
    void TearDown() override { std::filesystem::remove(path); }

    std::filesystem::path path;
};

PlayerStore::Record record() {
    PlayerStore::Record result;
    result.username = "alice";
    result.password_hash = "$test$hash";
    result.player_text = "{\"level\":3}";
    result.player_version = 2;
    return result;
}

TEST_F(PlayerStoreTest, SavesRestoresAndRemovesPlayer) {
    auto storage = PlayerStore::open(path.string());
    ASSERT_TRUE(storage);
    ASSERT_TRUE(storage.value()->save(record()));

    auto restored = storage.value()->restore("alice");
    ASSERT_TRUE(restored);
    EXPECT_EQ(restored.value().player_text, "{\"level\":3}");
    EXPECT_EQ(restored.value().player_version, 2);

    ASSERT_TRUE(storage.value()->remove("alice"));
    EXPECT_EQ(storage.value()->restore("alice").error().code, PlayerStore::ErrorCode::not_found);
}

TEST_F(PlayerStoreTest, LoginOnlyUpdatesTimestampForValidPassword) {
    auto storage = PlayerStore::open(path.string());
    ASSERT_TRUE(storage);
    ASSERT_TRUE(storage.value()->save(record()));

    const auto verifier = [](std::string_view password, std::string_view hash) {
        return password == "secret" && hash == "$test$hash";
    };
    EXPECT_EQ(storage.value()->login("alice", "wrong", verifier).error().code,
              PlayerStore::ErrorCode::authentication_failed);
    EXPECT_FALSE(storage.value()->restore("alice").value().has_logged_in);

    auto login = storage.value()->login("alice", "secret", verifier);
    ASSERT_TRUE(login);
    EXPECT_TRUE(login.value().has_logged_in);
    EXPECT_TRUE(storage.value()->restore("alice").value().has_logged_in);
}

TEST(PlayerStoreValidationTest, RejectsInvalidUsernames) {
    EXPECT_FALSE(PlayerStore::is_valid_username(""));
    EXPECT_FALSE(PlayerStore::is_valid_username("has space"));
    EXPECT_FALSE(PlayerStore::is_valid_username("has/slash"));
    EXPECT_TRUE(PlayerStore::is_valid_username("player_01"));
}

} // namespace