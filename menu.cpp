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
    std::map<int, MenuItem> sortedItems;
    auto range = m_menuItems.equal_range(menuName);
    for (auto it = range.first; it != range.second; ++it) {
        sortedItems[it->second.order] = it->second;
    }
    std::vector<MenuItem> rc;
    for (const auto& i : sortedItems) {
        rc.push_back(i.second);
    }
    return rc;
}

std::string Menu::getCurrentMenuName()
{
    return m_currentMenuName;
}

int Menu::getCurrentMenuSize()
{
    auto range = m_menuItems.equal_range(m_currentMenuName);
    return std::distance(range.first, range.second);
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

void Menu::selectCurrentItem()
{
    // TODO!
}

    int Menu::getCurrentlyHighlightedItem()
    {
        return m_currentlyHighlightedItem;
    }

} // namespace amaze
} // namespace marengo