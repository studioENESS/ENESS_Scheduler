// Shared schedule-list UI used by the GOOGLE and WELLESLEY features.
#pragma once

#if defined(FEATURE_GOOGLE) || defined(FEATURE_WELLESLEY)

#include <functional>
#include <string>

#include "../app_state.h"

void AddScheduleItem(bool bAddItem, bool bValidate);

// Draws the "+ Add Schedule Item" button (placed on the same line as the
// preceding Save button) followed by the schedule item list.
// itemPrefix supplies extra text for an item's tree-node label;
// drawItemBody draws the feature-specific controls inside an open node.
void DrawScheduleListUI(const std::function<std::string(SItemSchedule*)>& itemPrefix,
                        const std::function<void(SItemSchedule*)>& drawItemBody);

#endif // FEATURE_GOOGLE || FEATURE_WELLESLEY
