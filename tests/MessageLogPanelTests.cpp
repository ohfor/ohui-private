#include <catch2/catch_test_macros.hpp>

#include "ohui/message/MessageLogPanel.h"

using namespace ohui;
using namespace ohui::message;

struct LogHarness {
    MessageStream stream;
    MessageLogPanel panel;

    LogHarness() : panel(stream) {
        (void)stream.RegisterType({"quest", "Quest", true, 5.0f});
        (void)stream.RegisterType({"combat", "Combat", true, 5.0f});
        (void)stream.RegisterType({"system", "System", true, 5.0f});
    }

    uint64_t Publish(const std::string& content, const std::string& typeId = "quest",
                     const std::string& source = "", double gameTime = 0.0) {
        Message msg;
        msg.typeId = typeId;
        msg.content = content;
        msg.source = source;
        msg.gameTime = gameTime;
        return stream.Publish(std::move(msg));
    }

    void PublishMany(size_t count, const std::string& typeId = "quest") {
        for (size_t i = 0; i < count; ++i) {
            Publish("Message " + std::to_string(i + 1), typeId);
        }
    }
};

// ===========================================================================
// Tab management (7 tests)
// ===========================================================================

TEST_CASE("Default tab is 'all'", "[message-log]") {
    LogHarness h;
    CHECK(h.panel.ActiveTabId() == "all");
}

TEST_CASE("TabCount equals registered types + 1", "[message-log]") {
    LogHarness h;
    CHECK(h.panel.TabCount() == 4);  // all + quest + combat + system
}

TEST_CASE("GetTabIds starts with 'all'", "[message-log]") {
    LogHarness h;
    auto tabs = h.panel.GetTabIds();
    REQUIRE(tabs.size() >= 1);
    CHECK(tabs[0] == "all");
}

TEST_CASE("SelectTab valid type succeeds", "[message-log]") {
    LogHarness h;
    auto result = h.panel.SelectTab("quest");
    CHECK(result.has_value());
    CHECK(h.panel.ActiveTabId() == "quest");
}

TEST_CASE("SelectTab 'all' succeeds", "[message-log]") {
    LogHarness h;
    (void)h.panel.SelectTab("quest");
    auto result = h.panel.SelectTab("all");
    CHECK(result.has_value());
    CHECK(h.panel.ActiveTabId() == "all");
}

TEST_CASE("SelectTab unknown returns error", "[message-log]") {
    LogHarness h;
    auto result = h.panel.SelectTab("bogus");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("SelectTab resets page to 0", "[message-log]") {
    LogHarness h;
    h.panel.SetPageSize(2);
    h.PublishMany(10);
    (void)h.panel.GoToPage(3);
    CHECK(h.panel.CurrentPage() == 3);

    (void)h.panel.SelectTab("quest");
    CHECK(h.panel.CurrentPage() == 0);
}

// ===========================================================================
// Search (6 tests)
// ===========================================================================

TEST_CASE("Empty search shows all messages", "[message-log]") {
    LogHarness h;
    h.PublishMany(5);
    CHECK(h.panel.TotalFilteredCount() == 5);
}

TEST_CASE("Search filters by substring", "[message-log]") {
    LogHarness h;
    h.Publish("Dragon slain");
    h.Publish("Iron sword found");
    h.Publish("Dragon egg discovered");

    h.panel.SetSearchText("Dragon");
    CHECK(h.panel.TotalFilteredCount() == 2);
}

TEST_CASE("Search is case-insensitive", "[message-log]") {
    LogHarness h;
    h.Publish("DRAGON SLAIN");
    h.Publish("dragon egg");
    h.Publish("Iron sword");

    h.panel.SetSearchText("dragon");
    CHECK(h.panel.TotalFilteredCount() == 2);
}

TEST_CASE("No matches returns 0", "[message-log]") {
    LogHarness h;
    h.Publish("Hello");
    h.Publish("World");

    h.panel.SetSearchText("xyz");
    CHECK(h.panel.TotalFilteredCount() == 0);
}

TEST_CASE("ClearSearch restores all", "[message-log]") {
    LogHarness h;
    h.PublishMany(5);
    h.panel.SetSearchText("xyz");
    CHECK(h.panel.TotalFilteredCount() == 0);

    h.panel.ClearSearch();
    CHECK(h.panel.TotalFilteredCount() == 5);
}

TEST_CASE("SetSearchText resets page to 0", "[message-log]") {
    LogHarness h;
    h.panel.SetPageSize(2);
    h.PublishMany(10);
    (void)h.panel.GoToPage(3);
    CHECK(h.panel.CurrentPage() == 3);

    h.panel.SetSearchText("Message");
    CHECK(h.panel.CurrentPage() == 0);
}

// ===========================================================================
// Pagination (9 tests)
// ===========================================================================

TEST_CASE("Default page size is 50", "[message-log]") {
    LogHarness h;
    CHECK(h.panel.GetPageSize() == 50);
}

TEST_CASE("SetPageSize updates page size", "[message-log]") {
    LogHarness h;
    h.panel.SetPageSize(10);
    CHECK(h.panel.GetPageSize() == 10);
}

TEST_CASE("PageCount with 0 messages is 1", "[message-log]") {
    LogHarness h;
    CHECK(h.panel.PageCount() == 1);
}

TEST_CASE("Partial page count rounds up", "[message-log]") {
    LogHarness h;
    h.panel.SetPageSize(10);
    h.PublishMany(25);
    CHECK(h.panel.PageCount() == 3);  // 25/10 = 2.5 -> 3
}

TEST_CASE("Exact fit page count", "[message-log]") {
    LogHarness h;
    h.panel.SetPageSize(10);
    h.PublishMany(20);
    CHECK(h.panel.PageCount() == 2);
}

TEST_CASE("GetVisibleMessages returns correct window", "[message-log]") {
    LogHarness h;
    h.panel.SetPageSize(3);
    h.PublishMany(8);
    // Page 0: first 3 messages (most-recent-first from stream)
    auto msgs = h.panel.GetVisibleMessages();
    CHECK(msgs.size() == 3);

    // Page 1: next 3
    (void)h.panel.GoToPage(1);
    msgs = h.panel.GetVisibleMessages();
    CHECK(msgs.size() == 3);

    // Page 2: last 2
    (void)h.panel.GoToPage(2);
    msgs = h.panel.GetVisibleMessages();
    CHECK(msgs.size() == 2);
}

TEST_CASE("NextPage advances", "[message-log]") {
    LogHarness h;
    h.panel.SetPageSize(5);
    h.PublishMany(15);
    CHECK(h.panel.CurrentPage() == 0);

    auto result = h.panel.NextPage();
    CHECK(result.has_value());
    CHECK(h.panel.CurrentPage() == 1);
}

TEST_CASE("NextPage at last page returns error", "[message-log]") {
    LogHarness h;
    h.panel.SetPageSize(5);
    h.PublishMany(10);
    (void)h.panel.GoToPage(1);  // last page
    auto result = h.panel.NextPage();
    CHECK_FALSE(result.has_value());
}

TEST_CASE("PreviousPage at first page returns error", "[message-log]") {
    LogHarness h;
    auto result = h.panel.PreviousPage();
    CHECK_FALSE(result.has_value());
}

// ===========================================================================
// Tab + filter combined (4 tests)
// ===========================================================================

TEST_CASE("Type-filtered tab shows only that type", "[message-log]") {
    LogHarness h;
    h.Publish("Quest msg 1", "quest");
    h.Publish("Combat msg 1", "combat");
    h.Publish("Quest msg 2", "quest");

    (void)h.panel.SelectTab("quest");
    CHECK(h.panel.TotalFilteredCount() == 2);
}

TEST_CASE("'all' tab shows all messages", "[message-log]") {
    LogHarness h;
    h.Publish("Quest msg", "quest");
    h.Publish("Combat msg", "combat");
    h.Publish("System msg", "system");

    CHECK(h.panel.TotalFilteredCount() == 3);
}

TEST_CASE("Tab + search combined", "[message-log]") {
    LogHarness h;
    h.Publish("Dragon quest started", "quest");
    h.Publish("Dragon attacked", "combat");
    h.Publish("Iron sword quest", "quest");

    (void)h.panel.SelectTab("quest");
    h.panel.SetSearchText("Dragon");
    CHECK(h.panel.TotalFilteredCount() == 1);
}

TEST_CASE("Tab switch preserves search text", "[message-log]") {
    LogHarness h;
    h.Publish("Dragon quest", "quest");
    h.Publish("Dragon combat", "combat");

    h.panel.SetSearchText("Dragon");
    CHECK(h.panel.TotalFilteredCount() == 2);

    (void)h.panel.SelectTab("quest");
    CHECK(h.panel.GetSearchText() == "Dragon");
    CHECK(h.panel.TotalFilteredCount() == 1);
}

// ===========================================================================
// Copy formatting (6 tests)
// ===========================================================================

TEST_CASE("FormatMessage with source and gameTime", "[message-log]") {
    Message msg;
    msg.content = "Quest started";
    msg.source = "Whiterun";
    msg.gameTime = 12.5;
    auto formatted = MessageLogPanel::FormatMessage(msg);
    CHECK(formatted == "[Whiterun @ 12.5] Quest started");
}

TEST_CASE("FormatMessage empty source omitted", "[message-log]") {
    Message msg;
    msg.content = "Quest started";
    msg.gameTime = 12.5;
    auto formatted = MessageLogPanel::FormatMessage(msg);
    CHECK(formatted == "[@ 12.5] Quest started");
}

TEST_CASE("FormatMessage zero gameTime omitted", "[message-log]") {
    Message msg;
    msg.content = "Quest started";
    msg.source = "Whiterun";
    msg.gameTime = 0.0;
    auto formatted = MessageLogPanel::FormatMessage(msg);
    CHECK(formatted == "[Whiterun] Quest started");
}

TEST_CASE("FormatMessage neither source nor gameTime", "[message-log]") {
    Message msg;
    msg.content = "Quest started";
    auto formatted = MessageLogPanel::FormatMessage(msg);
    CHECK(formatted == "Quest started");
}

TEST_CASE("FormatVisibleMessages joins with newlines", "[message-log]") {
    LogHarness h;
    h.Publish("First message");
    h.Publish("Second message");
    auto formatted = h.panel.FormatVisibleMessages();
    // Messages are most-recent-first
    CHECK(formatted.find("First message") != std::string::npos);
    CHECK(formatted.find("Second message") != std::string::npos);
    CHECK(formatted.find("\n") != std::string::npos);
}

TEST_CASE("FormatAllTabMessages ignores pagination", "[message-log]") {
    LogHarness h;
    h.panel.SetPageSize(2);
    h.PublishMany(5);
    auto formatted = h.panel.FormatAllTabMessages();

    // Should contain all 5 messages despite page size of 2
    for (int i = 1; i <= 5; ++i) {
        CHECK(formatted.find("Message " + std::to_string(i)) != std::string::npos);
    }
}
