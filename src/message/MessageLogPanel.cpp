#include "ohui/message/MessageLogPanel.h"

#include <algorithm>
#include <sstream>

namespace ohui::message {

MessageLogPanel::MessageLogPanel(MessageStream& stream)
    : m_stream(stream) {}

std::vector<std::string> MessageLogPanel::GetTabIds() const {
    std::vector<std::string> tabs;
    tabs.push_back("all");
    for (const auto& typeId : m_stream.GetAllTypeIds()) {
        tabs.push_back(typeId);
    }
    return tabs;
}

size_t MessageLogPanel::TabCount() const {
    return 1 + m_stream.TypeCount();
}

Result<void> MessageLogPanel::SelectTab(std::string_view tabId) {
    if (tabId == "all") {
        m_activeTab = "all";
        m_currentPage = 0;
        return {};
    }
    if (!m_stream.HasType(tabId)) {
        return std::unexpected(Error{ErrorCode::InvalidState,
            "Unknown tab: " + std::string(tabId)});
    }
    m_activeTab = std::string(tabId);
    m_currentPage = 0;
    return {};
}

std::string_view MessageLogPanel::ActiveTabId() const {
    return m_activeTab;
}

size_t MessageLogPanel::ActiveTabIndex() const {
    auto tabs = GetTabIds();
    for (size_t i = 0; i < tabs.size(); ++i) {
        if (tabs[i] == m_activeTab) return i;
    }
    return 0;
}

void MessageLogPanel::SetSearchText(std::string_view text) {
    m_searchText = std::string(text);
    m_currentPage = 0;
}

std::string_view MessageLogPanel::GetSearchText() const {
    return m_searchText;
}

void MessageLogPanel::ClearSearch() {
    m_searchText.clear();
    m_currentPage = 0;
}

void MessageLogPanel::SetPageSize(size_t size) {
    m_pageSize = size > 0 ? size : 1;
    m_currentPage = 0;
}

size_t MessageLogPanel::GetPageSize() const {
    return m_pageSize;
}

size_t MessageLogPanel::CurrentPage() const {
    return m_currentPage;
}

size_t MessageLogPanel::PageCount() const {
    size_t total = TotalFilteredCount();
    if (total == 0) return 1;
    return (total + m_pageSize - 1) / m_pageSize;
}

size_t MessageLogPanel::TotalFilteredCount() const {
    return GetFilteredMessages().size();
}

Result<void> MessageLogPanel::GoToPage(size_t page) {
    if (page >= PageCount()) {
        return std::unexpected(Error{ErrorCode::InvalidState, "Page out of range"});
    }
    m_currentPage = page;
    return {};
}

Result<void> MessageLogPanel::NextPage() {
    if (m_currentPage + 1 >= PageCount()) {
        return std::unexpected(Error{ErrorCode::InvalidState, "Already on last page"});
    }
    ++m_currentPage;
    return {};
}

Result<void> MessageLogPanel::PreviousPage() {
    if (m_currentPage == 0) {
        return std::unexpected(Error{ErrorCode::InvalidState, "Already on first page"});
    }
    --m_currentPage;
    return {};
}

void MessageLogPanel::GoToFirstPage() {
    m_currentPage = 0;
}

void MessageLogPanel::GoToLastPage() {
    size_t pages = PageCount();
    m_currentPage = pages > 0 ? pages - 1 : 0;
}

std::vector<const Message*> MessageLogPanel::GetVisibleMessages() const {
    auto filtered = GetFilteredMessages();
    size_t start = m_currentPage * m_pageSize;
    if (start >= filtered.size()) return {};

    size_t end = std::min(start + m_pageSize, filtered.size());
    return {filtered.begin() + static_cast<ptrdiff_t>(start),
            filtered.begin() + static_cast<ptrdiff_t>(end)};
}

std::string MessageLogPanel::FormatMessage(const Message& msg) {
    std::ostringstream ss;

    bool hasSource = !msg.source.empty();
    bool hasGameTime = msg.gameTime > 0.0;

    if (hasSource && hasGameTime) {
        ss << "[" << msg.source << " @ " << msg.gameTime << "] " << msg.content;
    } else if (hasSource) {
        ss << "[" << msg.source << "] " << msg.content;
    } else if (hasGameTime) {
        ss << "[@ " << msg.gameTime << "] " << msg.content;
    } else {
        ss << msg.content;
    }

    return ss.str();
}

std::string MessageLogPanel::FormatVisibleMessages() const {
    auto msgs = GetVisibleMessages();
    std::ostringstream ss;
    for (size_t i = 0; i < msgs.size(); ++i) {
        if (i > 0) ss << "\n";
        ss << FormatMessage(*msgs[i]);
    }
    return ss.str();
}

std::string MessageLogPanel::FormatAllTabMessages() const {
    auto filtered = GetFilteredMessages();
    std::ostringstream ss;
    for (size_t i = 0; i < filtered.size(); ++i) {
        if (i > 0) ss << "\n";
        ss << FormatMessage(*filtered[i]);
    }
    return ss.str();
}

std::vector<const Message*> MessageLogPanel::GetFilteredMessages() const {
    // Get messages based on tab
    std::vector<const Message*> messages;
    if (m_activeTab == "all") {
        messages = m_stream.GetAllMessages();
    } else {
        messages = m_stream.GetMessages(m_activeTab);
    }

    // Apply search filter
    if (!m_searchText.empty()) {
        // Case-insensitive search
        std::string lowerSearch;
        lowerSearch.reserve(m_searchText.size());
        for (char c : m_searchText) {
            lowerSearch.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }

        std::vector<const Message*> filtered;
        for (const auto* msg : messages) {
            std::string lowerContent;
            lowerContent.reserve(msg->content.size());
            for (char c : msg->content) {
                lowerContent.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }
            if (lowerContent.find(lowerSearch) != std::string::npos) {
                filtered.push_back(msg);
            }
        }
        return filtered;
    }

    return messages;
}

}  // namespace ohui::message
