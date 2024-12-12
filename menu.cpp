#include "menu.h"

#include <map>

namespace marengo {
namespace amaze {

void Menu::addMenuItem(const std::string& menuName, MenuItem&& menuItem)
{
    m_menuItems.insert(std::make_pair(menuName, std::move(menuItem)));
}

std::vector<MenuItem> Menu::getMenuItems(const std::string& menuName)
{
    if (m_currentMenuName == menuName && !m_currentMenuItems.empty()) {
        return m_currentMenuItems;
    }
    m_currentMenuName = menuName;
    std::map<int, MenuItem> sortedItems;
    auto range = m_menuItems.equal_range(menuName);
    for (auto it = range.first; it != range.second; ++it) {
        sortedItems[it->second.order] = it->second;
    }
    std::vector<MenuItem> rc;
    for (const auto& i : sortedItems) {
        rc.push_back(i.second);
    }
    m_currentMenuItems = rc; // save copy for convenience
    m_currentlyHighlightedItem = 0;
    return rc;
}

std::string Menu::getCurrentMenuName()
{
    return m_currentMenuName;
}

int Menu::getCurrentMenuSize()
{
    return m_currentMenuItems.size();
}

void Menu::highlightNextItem()
{
    if (m_currentlyHighlightedItem < getCurrentMenuSize() - 1) {
        ++m_currentlyHighlightedItem;
    }
}

void Menu::highlightPreviousItem()
{
    if (m_currentlyHighlightedItem > 0) {
        --m_currentlyHighlightedItem;
    }
}

std::tuple<MenuItemId, std::optional<MenuItem>> Menu::selectCurrentItem()
{
    if (m_currentMenuItems[m_currentlyHighlightedItem].subMenuName.has_value()) {
        // User has selected an item with a submenu
        // TODO switch to new submenu - also need a way to return
        // We then return a value that the caller can ignore, and then
        // the new submenu will be displayed in the next frame
        getMenuItems(m_currentMenuItems[m_currentlyHighlightedItem].subMenuName.value());
        return { MenuItemId::SUBMENU, std::nullopt };
    }
    // This simply returns the name of the selected menu item
    // which the controller can use to take the appropriate action
    return { m_currentMenuItems[m_currentlyHighlightedItem].menuItemId,
             m_currentMenuItems[m_currentlyHighlightedItem] };
}

int Menu::getCurrentlyHighlightedItem()
{
    return m_currentlyHighlightedItem;
}

} // namespace amaze
} // namespace marengo