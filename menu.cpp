#include "menu.h"

#include <map>

namespace marengo {
namespace amaze {

void Menu::addMenuItem(const std::string& menuName, const MenuItem&& menuItem)
{
    m_menuItems.insert(std::make_pair(menuName, std::move(menuItem)));
}

std::vector<MenuItem> Menu::getMenuItems(const std::string& menuName)
{
    std::map<int, MenuItem> sortedItems;
    auto range = m_menuItems.equal_range(menuName);
    for (auto it = range.first; it != range.second; ++it) {
        sortedItems[it->second.order] = it->second;
    }
    std::vector<MenuItem> rc;
    for(const auto& i: sortedItems) {
        rc.push_back(i.second);
    }
    return rc;
}

} // namespace amaze
} // namespace marengo