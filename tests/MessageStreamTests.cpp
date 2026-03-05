#include <catch2/catch_test_macros.hpp>

#include "ohui/message/MessageStream.h"

using namespace ohui;
using namespace ohui::message;

static MessageTypeInfo MakeType(const std::string& id, const std::string& name = "") {
    return MessageTypeInfo{id, name.empty() ? id : name, true, 5.0f};
}

static Message MakeMsg(const std::string& typeId, const std::string& content = "test",
                        MessagePriority priority = MessagePriority::Normal) {
    Message msg;
    msg.typeId = typeId;
    msg.content = content;
    msg.priority = priority;
    return msg;
}

// --- Type registration ---

TEST_CASE("RegisterType succeeds for new type", "[message]") {
    MessageStream stream;
    auto result = stream.RegisterType(MakeType("notification", "Notifications"));
    CHECK(result.has_value());
    CHECK(stream.HasType("notification"));
}

TEST_CASE("RegisterType duplicate returns DuplicateRegistration", "[message]") {
    MessageStream stream;
    (void)stream.RegisterType(MakeType("notification"));
    auto result = stream.RegisterType(MakeType("notification"));
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::DuplicateRegistration);
}

TEST_CASE("HasType/GetTypeInfo correct", "[message]") {
    MessageStream stream;
    (void)stream.RegisterType(MakeType("quest", "Quest Updates"));
    CHECK(stream.HasType("quest"));
    CHECK(!stream.HasType("nonexistent"));

    auto* info = stream.GetTypeInfo("quest");
    REQUIRE(info != nullptr);
    CHECK(info->displayName == "Quest Updates");
    CHECK(stream.GetTypeInfo("nonexistent") == nullptr);
}

TEST_CASE("GetAllTypeIds returns all registered", "[message]") {
    MessageStream stream;
    (void)stream.RegisterType(MakeType("a"));
    (void)stream.RegisterType(MakeType("b"));
    (void)stream.RegisterType(MakeType("c"));

    auto ids = stream.GetAllTypeIds();
    CHECK(ids.size() == 3);
}

// --- Publishing ---

TEST_CASE("Publish assigns auto-incrementing ID", "[message]") {
    MessageStream stream;
    uint64_t id1 = stream.Publish(MakeMsg("notification", "first"));
    uint64_t id2 = stream.Publish(MakeMsg("notification", "second"));
    CHECK(id2 == id1 + 1);
}

TEST_CASE("Publish stores message retrievable by ID", "[message]") {
    MessageStream stream;
    uint64_t id = stream.Publish(MakeMsg("quest", "New quest added"));
    auto* msg = stream.GetMessage(id);
    REQUIRE(msg != nullptr);
    CHECK(msg->content == "New quest added");
    CHECK(msg->typeId == "quest");
}

TEST_CASE("Publish with unregistered type still stores (permissive)", "[message]") {
    MessageStream stream;
    // No types registered, but publish should still work
    uint64_t id = stream.Publish(MakeMsg("unregistered_type", "content"));
    CHECK(stream.GetMessage(id) != nullptr);
    CHECK(stream.MessageCount() == 1);
}

// --- Subscribing ---

TEST_CASE("Subscribe succeeds for new subscriber", "[message]") {
    MessageStream stream;
    auto result = stream.Subscribe("sub1", {}, [](const Message&) {});
    CHECK(result.has_value());
}

TEST_CASE("Subscribe duplicate ID returns DuplicateRegistration", "[message]") {
    MessageStream stream;
    (void)stream.Subscribe("sub1", {}, [](const Message&) {});
    auto result = stream.Subscribe("sub1", {}, [](const Message&) {});
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::DuplicateRegistration);
}

TEST_CASE("Unsubscribe removes subscriber", "[message]") {
    MessageStream stream;
    (void)stream.Subscribe("sub1", {}, [](const Message&) {});
    CHECK(stream.HasSubscriber("sub1"));

    auto result = stream.Unsubscribe("sub1");
    CHECK(result.has_value());
    CHECK(!stream.HasSubscriber("sub1"));
}

TEST_CASE("Unsubscribe unknown ID returns NotRegistered", "[message]") {
    MessageStream stream;
    auto result = stream.Unsubscribe("nonexistent");
    REQUIRE(!result.has_value());
    CHECK(result.error().code == ErrorCode::NotRegistered);
}

TEST_CASE("Subscriber callback fires on Publish", "[message]") {
    MessageStream stream;
    std::string received;
    (void)stream.Subscribe("sub1", {}, [&](const Message& msg) {
        received = msg.content;
    });

    stream.Publish(MakeMsg("notification", "hello"));
    CHECK(received == "hello");
}

TEST_CASE("Subscriber with type filter receives only matching types", "[message]") {
    MessageStream stream;
    std::vector<std::string> received;

    SubscriptionFilter filter;
    filter.typeIds = {"quest"};

    (void)stream.Subscribe("sub1", filter, [&](const Message& msg) {
        received.push_back(msg.content);
    });

    stream.Publish(MakeMsg("notification", "notif1"));
    stream.Publish(MakeMsg("quest", "quest1"));
    stream.Publish(MakeMsg("notification", "notif2"));
    stream.Publish(MakeMsg("quest", "quest2"));

    CHECK(received.size() == 2);
    CHECK(received[0] == "quest1");
    CHECK(received[1] == "quest2");
}

TEST_CASE("Subscriber with empty filter receives all types", "[message]") {
    MessageStream stream;
    size_t count = 0;
    (void)stream.Subscribe("sub1", {}, [&](const Message&) { ++count; });

    stream.Publish(MakeMsg("a", "1"));
    stream.Publish(MakeMsg("b", "2"));
    stream.Publish(MakeMsg("c", "3"));

    CHECK(count == 3);
}

TEST_CASE("Subscriber with priority filter receives only matching+ priority", "[message]") {
    MessageStream stream;
    std::vector<std::string> received;

    SubscriptionFilter filter;
    filter.minPriority = MessagePriority::High;

    (void)stream.Subscribe("sub1", filter, [&](const Message& msg) {
        received.push_back(msg.content);
    });

    stream.Publish(MakeMsg("n", "low", MessagePriority::Low));
    stream.Publish(MakeMsg("n", "normal", MessagePriority::Normal));
    stream.Publish(MakeMsg("n", "high", MessagePriority::High));
    stream.Publish(MakeMsg("n", "critical", MessagePriority::Critical));

    CHECK(received.size() == 2);
    CHECK(received[0] == "high");
    CHECK(received[1] == "critical");
}

TEST_CASE("Multiple subscribers: each receives independently", "[message]") {
    MessageStream stream;
    size_t count1 = 0, count2 = 0;

    SubscriptionFilter f1;
    f1.typeIds = {"a"};
    SubscriptionFilter f2;
    f2.typeIds = {"b"};

    (void)stream.Subscribe("s1", f1, [&](const Message&) { ++count1; });
    (void)stream.Subscribe("s2", f2, [&](const Message&) { ++count2; });

    stream.Publish(MakeMsg("a", "x"));
    stream.Publish(MakeMsg("b", "y"));
    stream.Publish(MakeMsg("a", "z"));

    CHECK(count1 == 2);
    CHECK(count2 == 1);
}

// --- Query ---

TEST_CASE("GetMessage returns correct message by ID", "[message]") {
    MessageStream stream;
    uint64_t id1 = stream.Publish(MakeMsg("a", "first"));
    uint64_t id2 = stream.Publish(MakeMsg("b", "second"));

    CHECK(stream.GetMessage(id1)->content == "first");
    CHECK(stream.GetMessage(id2)->content == "second");
    CHECK(stream.GetMessage(999) == nullptr);
}

TEST_CASE("GetMessages filters by type", "[message]") {
    MessageStream stream;
    stream.Publish(MakeMsg("a", "1"));
    stream.Publish(MakeMsg("b", "2"));
    stream.Publish(MakeMsg("a", "3"));

    auto msgs = stream.GetMessages("a");
    CHECK(msgs.size() == 2);
}

TEST_CASE("GetMessages with limit returns most recent N", "[message]") {
    MessageStream stream;
    stream.Publish(MakeMsg("a", "old"));
    stream.Publish(MakeMsg("a", "mid"));
    stream.Publish(MakeMsg("a", "new"));

    auto msgs = stream.GetMessages("a", 2);
    REQUIRE(msgs.size() == 2);
    CHECK(msgs[0]->content == "new");
    CHECK(msgs[1]->content == "mid");
}

TEST_CASE("GetAllMessages returns all in order", "[message]") {
    MessageStream stream;
    stream.Publish(MakeMsg("a", "1"));
    stream.Publish(MakeMsg("b", "2"));
    stream.Publish(MakeMsg("c", "3"));

    auto msgs = stream.GetAllMessages();
    CHECK(msgs.size() == 3);
    // Returned most-recent-first for consistency with limited queries
    CHECK(msgs[0]->content == "3");
    CHECK(msgs[2]->content == "1");
}

TEST_CASE("MessageCount total and per-type correct", "[message]") {
    MessageStream stream;
    stream.Publish(MakeMsg("a", "1"));
    stream.Publish(MakeMsg("a", "2"));
    stream.Publish(MakeMsg("b", "3"));

    CHECK(stream.MessageCount() == 3);
    CHECK(stream.MessageCount("a") == 2);
    CHECK(stream.MessageCount("b") == 1);
    CHECK(stream.MessageCount("c") == 0);
}

// --- Session management ---

TEST_CASE("Clear removes all messages, resets ID counter", "[message]") {
    MessageStream stream;
    stream.Publish(MakeMsg("a", "1"));
    stream.Publish(MakeMsg("b", "2"));

    stream.Clear();
    CHECK(stream.MessageCount() == 0);

    // New messages start from ID 1 again
    uint64_t id = stream.Publish(MakeMsg("a", "new"));
    CHECK(id == 1);
}

TEST_CASE("SetMaxMessages: oldest messages evicted when limit exceeded", "[message]") {
    MessageStream stream;
    stream.SetMaxMessages(3);

    stream.Publish(MakeMsg("a", "1"));
    stream.Publish(MakeMsg("a", "2"));
    stream.Publish(MakeMsg("a", "3"));
    stream.Publish(MakeMsg("a", "4"));

    CHECK(stream.MessageCount() == 3);
    // Oldest (1) should be evicted
    auto msgs = stream.GetAllMessages();
    CHECK(msgs[0]->content == "4");
    CHECK(msgs[2]->content == "2");
}

TEST_CASE("Rolling eviction maintains order", "[message]") {
    MessageStream stream;
    stream.SetMaxMessages(2);

    stream.Publish(MakeMsg("a", "1"));
    stream.Publish(MakeMsg("a", "2"));
    stream.Publish(MakeMsg("a", "3"));
    stream.Publish(MakeMsg("a", "4"));
    stream.Publish(MakeMsg("a", "5"));

    CHECK(stream.MessageCount() == 2);
    auto msgs = stream.GetAllMessages();
    CHECK(msgs[0]->content == "5");
    CHECK(msgs[1]->content == "4");
}

// --- Serialization ---

TEST_CASE("SerializeRecent/DeserializeInto round-trip", "[message]") {
    MessageStream source;
    Message m1 = MakeMsg("quest", "New quest!", MessagePriority::High);
    m1.speaker = "Jarl";
    m1.questId = "MQ101";
    m1.gameTime = 100.5;
    m1.realTime = 45.2;
    m1.lifetimeHint = 10.0f;
    m1.source = "Main Quest";
    source.Publish(m1);

    Message m2 = MakeMsg("notification", "Item added");
    m2.source = "System";
    source.Publish(m2);

    auto data = source.SerializeRecent(10);

    MessageStream dest;
    auto result = dest.DeserializeInto(data);
    REQUIRE(result.has_value());
    CHECK(*result == 2);
    CHECK(dest.MessageCount() == 2);

    // Verify content preserved
    auto msgs = dest.GetAllMessages();
    // Most recent first
    CHECK(msgs[0]->content == "Item added");
    CHECK(msgs[1]->content == "New quest!");
    CHECK(msgs[1]->speaker == "Jarl");
    CHECK(msgs[1]->questId == "MQ101");
    CHECK(msgs[1]->priority == MessagePriority::High);
}

TEST_CASE("Deserialized messages get new IDs (no collision)", "[message]") {
    MessageStream source;
    source.Publish(MakeMsg("a", "1"));
    source.Publish(MakeMsg("a", "2"));
    auto data = source.SerializeRecent(2);

    MessageStream dest;
    // Publish something first so IDs are offset
    dest.Publish(MakeMsg("b", "existing"));

    (void)dest.DeserializeInto(data);

    // All three messages should have unique IDs
    CHECK(dest.MessageCount() == 3);
    auto msgs = dest.GetAllMessages();
    // Check no duplicate IDs
    std::vector<uint64_t> ids;
    for (auto* m : msgs) ids.push_back(m->id);
    std::sort(ids.begin(), ids.end());
    auto uniqueEnd = std::unique(ids.begin(), ids.end());
    CHECK(uniqueEnd == ids.end());
}

// --- End-to-end ---

TEST_CASE("Custom type registration + publish + subscribe end-to-end", "[message]") {
    MessageStream stream;

    // Register types
    (void)stream.RegisterType(MakeType("subtitle_interactive", "Subtitles"));
    (void)stream.RegisterType(MakeType("notification", "Notifications"));

    // Subscribe to subtitles only
    std::vector<std::string> subtitles;
    SubscriptionFilter filter;
    filter.typeIds = {"subtitle_interactive"};
    (void)stream.Subscribe("subtitle_panel", filter, [&](const Message& msg) {
        subtitles.push_back(msg.content);
    });

    // Publish mixed messages
    Message sub;
    sub.typeId = "subtitle_interactive";
    sub.content = "I used to be an adventurer...";
    sub.speaker = "Guard";
    stream.Publish(sub);

    stream.Publish(MakeMsg("notification", "Level up!"));

    sub.content = "...then I took an arrow to the knee.";
    stream.Publish(sub);

    CHECK(subtitles.size() == 2);
    CHECK(stream.MessageCount() == 3);
    CHECK(stream.MessageCount("subtitle_interactive") == 2);
    CHECK(stream.TypeCount() == 2);
}

TEST_CASE("HasSubscriber/SubscriberCount correct", "[message]") {
    MessageStream stream;
    CHECK(stream.SubscriberCount() == 0);
    CHECK(!stream.HasSubscriber("s1"));

    (void)stream.Subscribe("s1", {}, [](const Message&) {});
    CHECK(stream.SubscriberCount() == 1);
    CHECK(stream.HasSubscriber("s1"));

    (void)stream.Subscribe("s2", {}, [](const Message&) {});
    CHECK(stream.SubscriberCount() == 2);

    (void)stream.Unsubscribe("s1");
    CHECK(stream.SubscriberCount() == 1);
    CHECK(!stream.HasSubscriber("s1"));
}
