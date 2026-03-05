#include "ohui/mcm/MCM2DefinitionDiff.h"

#include <unordered_map>
#include <unordered_set>

namespace ohui::mcm {

bool MCM2DefinitionDelta::HasDestructiveChanges() const {
    if (mcmIdChanged) return true;
    for (const auto& c : controlChanges) {
        if (c.type == DiffChangeType::Destructive) return true;
    }
    return false;
}

bool MCM2DefinitionDelta::HasAnyChanges() const {
    return mcmIdChanged || displayNameChanged || versionChanged ||
           !pageChanges.empty() || !sectionChanges.empty() || !controlChanges.empty();
}

size_t MCM2DefinitionDelta::DestructiveChangeCount() const {
    size_t count = 0;
    if (mcmIdChanged) ++count;
    for (const auto& c : controlChanges) {
        if (c.type == DiffChangeType::Destructive) ++count;
    }
    return count;
}

// --- Helper to compare control properties ---

namespace {

std::string ControlPropsToString(const MCM2ControlDef& ctrl) {
    return std::visit([&ctrl](const auto& props) -> std::string {
        using T = std::decay_t<decltype(props)>;
        std::string result;
        if constexpr (std::is_same_v<T, MCM2ToggleProps>) {
            result = "default=" + std::string(props.defaultValue ? "true" : "false");
        } else if constexpr (std::is_same_v<T, MCM2SliderProps>) {
            result = "min=" + std::to_string(props.minValue) +
                     ",max=" + std::to_string(props.maxValue) +
                     ",step=" + std::to_string(props.stepSize) +
                     ",default=" + std::to_string(props.defaultValue);
            if (!props.formatString.empty()) result += ",format=" + props.formatString;
        } else if constexpr (std::is_same_v<T, MCM2DropdownProps>) {
            result = "options=[";
            for (size_t i = 0; i < props.options.size(); ++i) {
                if (i > 0) result += ",";
                result += props.options[i];
            }
            result += "],default=" + props.defaultValue;
        } else if constexpr (std::is_same_v<T, MCM2KeyBindProps>) {
            result = "default=" + std::to_string(props.defaultValue);
        } else if constexpr (std::is_same_v<T, MCM2ColourProps>) {
            result = "default=" + std::to_string(props.defaultValue);
        } else if constexpr (std::is_same_v<T, MCM2TextProps>) {
            result = "default=" + props.defaultValue;
        } else {
            result = "";
        }
        return result;
    }, ctrl.properties);
}

void DiffControls(const std::string& pageId, const std::string& sectionId,
                  const std::vector<MCM2ControlDef>& oldCtrls,
                  const std::vector<MCM2ControlDef>& newCtrls,
                  std::vector<ControlChange>& changes) {
    // Build maps by ID
    std::unordered_map<std::string, const MCM2ControlDef*> oldMap;
    std::unordered_map<std::string, const MCM2ControlDef*> newMap;

    for (const auto& c : oldCtrls) {
        if (!c.id.empty()) oldMap[c.id] = &c;
    }
    for (const auto& c : newCtrls) {
        if (!c.id.empty()) newMap[c.id] = &c;
    }

    // Added controls
    for (const auto& [id, ctrl] : newMap) {
        if (!oldMap.contains(id)) {
            changes.push_back({DiffChangeType::Added, pageId, sectionId, id,
                               "control added", "", "", ""});
        }
    }

    // Removed controls
    for (const auto& [id, ctrl] : oldMap) {
        if (!newMap.contains(id)) {
            changes.push_back({DiffChangeType::Removed, pageId, sectionId, id,
                               "control removed", "", "", ""});
        }
    }

    // Modified controls
    for (const auto& [id, newCtrl] : newMap) {
        auto oldIt = oldMap.find(id);
        if (oldIt == oldMap.end()) continue;

        const auto* oldCtrl = oldIt->second;

        if (oldCtrl->label != newCtrl->label) {
            changes.push_back({DiffChangeType::Modified, pageId, sectionId, id,
                               "label changed", "label", oldCtrl->label, newCtrl->label});
        }
        if (oldCtrl->description != newCtrl->description) {
            changes.push_back({DiffChangeType::Modified, pageId, sectionId, id,
                               "description changed", "description",
                               oldCtrl->description, newCtrl->description});
        }
        if (oldCtrl->conditionExpr != newCtrl->conditionExpr) {
            changes.push_back({DiffChangeType::Modified, pageId, sectionId, id,
                               "condition changed", "condition",
                               oldCtrl->conditionExpr, newCtrl->conditionExpr});
        }
        if (oldCtrl->onChange != newCtrl->onChange) {
            changes.push_back({DiffChangeType::Modified, pageId, sectionId, id,
                               "onChange changed", "onChange",
                               oldCtrl->onChange, newCtrl->onChange});
        }

        // Compare type-specific properties
        auto oldProps = ControlPropsToString(*oldCtrl);
        auto newProps = ControlPropsToString(*newCtrl);
        if (oldProps != newProps) {
            changes.push_back({DiffChangeType::Modified, pageId, sectionId, id,
                               "properties changed", "properties", oldProps, newProps});
        }
    }

    // Positional heuristic for ID changes: if a control was removed at position N
    // and a different one was added at position N, flag as destructive (ID rename)
    if (oldCtrls.size() == newCtrls.size()) {
        for (size_t i = 0; i < oldCtrls.size(); ++i) {
            if (oldCtrls[i].id.empty() || newCtrls[i].id.empty()) continue;
            if (oldCtrls[i].id != newCtrls[i].id) {
                // Check if both old and new IDs are not present in the other set
                if (!newMap.contains(oldCtrls[i].id) && !oldMap.contains(newCtrls[i].id)) {
                    // This is likely an ID rename — flag as destructive
                    // Remove the Added/Removed entries and add a Destructive entry
                    std::erase_if(changes, [&](const ControlChange& c) {
                        return (c.controlId == oldCtrls[i].id && c.type == DiffChangeType::Removed) ||
                               (c.controlId == newCtrls[i].id && c.type == DiffChangeType::Added);
                    });
                    changes.push_back({DiffChangeType::Destructive, pageId, sectionId,
                                       oldCtrls[i].id,
                                       "control ID changed from '" + oldCtrls[i].id +
                                       "' to '" + newCtrls[i].id + "'",
                                       "id", oldCtrls[i].id, newCtrls[i].id});
                }
            }
        }
    }
}

void DiffSections(const std::string& pageId,
                  const std::vector<MCM2SectionDef>& oldSections,
                  const std::vector<MCM2SectionDef>& newSections,
                  std::vector<SectionChange>& sectionChanges,
                  std::vector<ControlChange>& controlChanges) {
    std::unordered_map<std::string, const MCM2SectionDef*> oldMap;
    std::unordered_map<std::string, const MCM2SectionDef*> newMap;

    for (const auto& s : oldSections) oldMap[s.id] = &s;
    for (const auto& s : newSections) newMap[s.id] = &s;

    for (const auto& [id, sec] : newMap) {
        if (!oldMap.contains(id)) {
            sectionChanges.push_back({DiffChangeType::Added, pageId, id, "section added"});
        }
    }

    for (const auto& [id, sec] : oldMap) {
        if (!newMap.contains(id)) {
            sectionChanges.push_back({DiffChangeType::Removed, pageId, id, "section removed"});
        }
    }

    // Diff controls within matching sections
    for (const auto& [id, newSec] : newMap) {
        auto oldIt = oldMap.find(id);
        if (oldIt == oldMap.end()) continue;
        DiffControls(pageId, id, oldIt->second->controls, newSec->controls, controlChanges);
    }
}

}  // anonymous namespace

MCM2DefinitionDelta ComputeDiff(const MCM2Definition& oldDef, const MCM2Definition& newDef) {
    MCM2DefinitionDelta delta;

    // MCM ID change
    if (oldDef.id != newDef.id) {
        delta.mcmIdChanged = true;
        delta.oldMcmId = oldDef.id;
        delta.newMcmId = newDef.id;
    }

    // Display name change
    if (oldDef.displayName != newDef.displayName) {
        delta.displayNameChanged = true;
        delta.oldDisplayName = oldDef.displayName;
        delta.newDisplayName = newDef.displayName;
    }

    // Version change
    if (oldDef.version.major != newDef.version.major ||
        oldDef.version.minor != newDef.version.minor ||
        oldDef.version.patch != newDef.version.patch) {
        delta.versionChanged = true;
    }

    // Diff pages
    std::unordered_map<std::string, const MCM2PageDef*> oldPages;
    std::unordered_map<std::string, const MCM2PageDef*> newPages;

    for (const auto& p : oldDef.pages) oldPages[p.id] = &p;
    for (const auto& p : newDef.pages) newPages[p.id] = &p;

    for (const auto& [id, page] : newPages) {
        if (!oldPages.contains(id)) {
            delta.pageChanges.push_back({DiffChangeType::Added, id, "page added"});
        }
    }

    for (const auto& [id, page] : oldPages) {
        if (!newPages.contains(id)) {
            delta.pageChanges.push_back({DiffChangeType::Removed, id, "page removed"});
        }
    }

    // Diff sections/controls within matching pages
    for (const auto& [id, newPage] : newPages) {
        auto oldIt = oldPages.find(id);
        if (oldIt == oldPages.end()) continue;
        DiffSections(id, oldIt->second->sections, newPage->sections,
                     delta.sectionChanges, delta.controlChanges);
    }

    return delta;
}

}  // namespace ohui::mcm
