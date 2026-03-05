#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ohui/cosave/PersistenceAPI.h"
#include "MockFileSystem.h"

using namespace ohui;
using namespace ohui::cosave;
using Catch::Matchers::WithinAbs;

static PersistenceAPI MakeAPI() {
    static test::MockFileSystem fs;
    static CosaveManager mgr(fs);
    return PersistenceAPI(mgr);
}

TEST_CASE("Register and IsRegistered", "[persistence]") {
    auto api = MakeAPI();
    CHECK(!api.IsRegistered("TestMod"));
    REQUIRE(api.Register("TestMod").has_value());
    CHECK(api.IsRegistered("TestMod"));
}

TEST_CASE("Double registration returns DuplicateRegistration", "[persistence]") {
    auto api = MakeAPI();
    REQUIRE(api.Register("DupMod").has_value());
    auto result = api.Register("DupMod");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::DuplicateRegistration);
}

TEST_CASE("SetBool/GetBool round-trip", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModBool");
    REQUIRE(api.SetBool("ModBool", "flag", true).has_value());
    CHECK(api.GetBool("ModBool", "flag") == true);
    CHECK(api.GetBool("ModBool", "flag", false) == true);
}

TEST_CASE("SetInt/GetInt round-trip", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModInt");
    REQUIRE(api.SetInt("ModInt", "count", 42).has_value());
    CHECK(api.GetInt("ModInt", "count") == 42);
}

TEST_CASE("SetFloat/GetFloat round-trip", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModFloat");
    REQUIRE(api.SetFloat("ModFloat", "ratio", 3.14f).has_value());
    CHECK_THAT(static_cast<double>(api.GetFloat("ModFloat", "ratio")), WithinAbs(3.14, 0.001));
}

TEST_CASE("SetString/GetString round-trip", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModStr");
    REQUIRE(api.SetString("ModStr", "name", "Hello World").has_value());
    CHECK(api.GetString("ModStr", "name") == "Hello World");
}

TEST_CASE("SetBytes/GetBytes round-trip", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModBytes");
    std::vector<uint8_t> data{0xDE, 0xAD, 0xBE, 0xEF};
    REQUIRE(api.SetBytes("ModBytes", "blob", data).has_value());
    CHECK(api.GetBytes("ModBytes", "blob") == data);
}

TEST_CASE("Get returns default for missing key", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModDef");
    CHECK(api.GetBool("ModDef", "missing", true) == true);
    CHECK(api.GetInt("ModDef", "missing", -1) == -1);
    CHECK_THAT(static_cast<double>(api.GetFloat("ModDef", "missing", 9.9f)), WithinAbs(9.9, 0.001));
    CHECK(api.GetString("ModDef", "missing", "fallback") == "fallback");
    CHECK(api.GetBytes("ModDef", "missing").empty());
}

TEST_CASE("Get returns default for unregistered mod", "[persistence]") {
    auto api = MakeAPI();
    CHECK(api.GetBool("UnknownMod", "key", true) == true);
    CHECK(api.GetInt("UnknownMod", "key", 99) == 99);
    CHECK(api.GetString("UnknownMod", "key", "def") == "def");
}

TEST_CASE("Set fails for unregistered mod with NotRegistered", "[persistence]") {
    auto api = MakeAPI();
    auto result = api.SetBool("UnregMod", "key", true);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::NotRegistered);
}

TEST_CASE("Namespace isolation: different mods same key", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModA");
    api.Register("ModB");
    api.SetInt("ModA", "val", 10);
    api.SetInt("ModB", "val", 20);
    CHECK(api.GetInt("ModA", "val") == 10);
    CHECK(api.GetInt("ModB", "val") == 20);
}

TEST_CASE("Has true/false", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModHas");
    CHECK(!api.Has("ModHas", "key"));
    api.SetInt("ModHas", "key", 1);
    CHECK(api.Has("ModHas", "key"));
}

TEST_CASE("Delete removes key", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModDel");
    api.SetInt("ModDel", "key", 1);
    CHECK(api.Has("ModDel", "key"));
    REQUIRE(api.Delete("ModDel", "key").has_value());
    CHECK(!api.Has("ModDel", "key"));
}

TEST_CASE("ClearAll removes all keys for one mod, leaves others", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModClearA");
    api.Register("ModClearB");
    api.SetInt("ModClearA", "x", 1);
    api.SetInt("ModClearA", "y", 2);
    api.SetInt("ModClearB", "z", 3);
    REQUIRE(api.ClearAll("ModClearA").has_value());
    CHECK(!api.Has("ModClearA", "x"));
    CHECK(!api.Has("ModClearA", "y"));
    CHECK(api.Has("ModClearB", "z"));
}

TEST_CASE("Entry size limit enforced", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModBig");
    std::vector<uint8_t> huge(65 * 1024, 0xAA); // >64KB
    auto result = api.SetBytes("ModBig", "big", huge);
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::SizeLimitExceeded);
}

TEST_CASE("Mod total size limit enforced", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModTotal");
    // Fill up: 1MB = 1048576 bytes. Use 64KB chunks = 16 fit exactly, 17th should fail
    std::vector<uint8_t> chunk(64 * 1024, 0xBB); // 64KB each (exact entry size limit)
    // 16 * 64KB = 1MB exactly, but the check is > not >=, so 16 should fit
    for (int i = 0; i < 16; ++i) {
        auto r = api.SetBytes("ModTotal", "chunk" + std::to_string(i), chunk);
        if (!r.has_value()) {
            CHECK(r.error().code == ErrorCode::SizeLimitExceeded);
            return;
        }
    }
    // 17th should exceed 1MB total
    auto lastResult = api.SetBytes("ModTotal", "overflow", chunk);
    CHECK(!lastResult.has_value());
}

TEST_CASE("Version get/set", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModVer");
    CHECK(api.GetVersion("ModVer") == 0);
    REQUIRE(api.SetVersion("ModVer", 5).has_value());
    CHECK(api.GetVersion("ModVer") == 5);
}

TEST_CASE("SerializeAll then DeserializeAll round-trip", "[persistence]") {
    auto api = MakeAPI();
    api.Register("SerMod");
    api.SetBool("SerMod", "b", true);
    api.SetInt("SerMod", "i", 123);
    api.SetFloat("SerMod", "f", 2.5f);
    api.SetString("SerMod", "s", "hello");
    api.SetBytes("SerMod", "raw", std::vector<uint8_t>{0x01, 0x02});
    api.SetVersion("SerMod", 7);

    auto bytes = api.SerializeAll();
    REQUIRE(!bytes.empty());

    // Create fresh API and deserialize
    auto api2 = MakeAPI();
    REQUIRE(api2.DeserializeAll(bytes).has_value());

    CHECK(api2.IsRegistered("SerMod"));
    CHECK(api2.GetBool("SerMod", "b") == true);
    CHECK(api2.GetInt("SerMod", "i") == 123);
    CHECK_THAT(static_cast<double>(api2.GetFloat("SerMod", "f")), WithinAbs(2.5, 0.001));
    CHECK(api2.GetString("SerMod", "s") == "hello");
    CHECK(api2.GetBytes("SerMod", "raw") == std::vector<uint8_t>{0x01, 0x02});
    CHECK(api2.GetVersion("SerMod") == 7);
}

TEST_CASE("RevertAll clears all data", "[persistence]") {
    auto api = MakeAPI();
    api.Register("RevMod");
    api.SetInt("RevMod", "val", 42);
    api.RevertAll();
    CHECK(!api.IsRegistered("RevMod"));
}

TEST_CASE("Overwrite existing key updates value", "[persistence]") {
    auto api = MakeAPI();
    api.Register("ModOverwrite");
    api.SetInt("ModOverwrite", "k", 1);
    CHECK(api.GetInt("ModOverwrite", "k") == 1);
    api.SetInt("ModOverwrite", "k", 2);
    CHECK(api.GetInt("ModOverwrite", "k") == 2);
}
