#pragma once

#include "ohui/mcm/MCM2Types.h"

#include <string>
#include <vector>

namespace ohui::mcm {

enum class DiffChangeType { Added, Removed, Modified, Destructive };

struct ControlChange {
    DiffChangeType type;
    std::string pageId;
    std::string sectionId;
    std::string controlId;
    std::string description;
    std::string fieldName;
    std::string oldValue;
    std::string newValue;
};

struct PageChange {
    DiffChangeType type;
    std::string pageId;
    std::string description;
};

struct SectionChange {
    DiffChangeType type;
    std::string pageId;
    std::string sectionId;
    std::string description;
};

struct MCM2DefinitionDelta {
    bool mcmIdChanged{false};
    std::string oldMcmId, newMcmId;
    bool displayNameChanged{false};
    std::string oldDisplayName, newDisplayName;
    bool versionChanged{false};

    std::vector<PageChange> pageChanges;
    std::vector<SectionChange> sectionChanges;
    std::vector<ControlChange> controlChanges;

    bool HasDestructiveChanges() const;
    bool HasAnyChanges() const;
    size_t DestructiveChangeCount() const;
};

MCM2DefinitionDelta ComputeDiff(const MCM2Definition& oldDef, const MCM2Definition& newDef);

}  // namespace ohui::mcm
