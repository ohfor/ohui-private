#pragma once

#include "ohui/message/MessageStream.h"
#include "ohui/core/Result.h"

#include <string>
#include <string_view>
#include <vector>

namespace ohui::message {

class MessageLogPanel {
public:
    explicit MessageLogPanel(MessageStream& stream);

    // Tabs: "all" + one per registered MessageTypeInfo
    std::vector<std::string> GetTabIds() const;
    size_t TabCount() const;
    Result<void> SelectTab(std::string_view tabId);
    std::string_view ActiveTabId() const;
    size_t ActiveTabIndex() const;

    // Search: case-insensitive substring on content
    void SetSearchText(std::string_view text);
    std::string_view GetSearchText() const;
    void ClearSearch();

    // Pagination
    void SetPageSize(size_t size);
    size_t GetPageSize() const;
    size_t CurrentPage() const;
    size_t PageCount() const;
    size_t TotalFilteredCount() const;
    Result<void> GoToPage(size_t page);
    Result<void> NextPage();
    Result<void> PreviousPage();
    void GoToFirstPage();
    void GoToLastPage();

    // View
    std::vector<const Message*> GetVisibleMessages() const;

    // Copy formatting
    static std::string FormatMessage(const Message& msg);
    std::string FormatVisibleMessages() const;
    std::string FormatAllTabMessages() const;

private:
    std::vector<const Message*> GetFilteredMessages() const;

    MessageStream& m_stream;
    std::string m_activeTab{"all"};
    std::string m_searchText;
    size_t m_pageSize{50};
    size_t m_currentPage{0};
};

}  // namespace ohui::message
