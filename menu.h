#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace marengo {
namespace amaze {

enum class MenuItemId {
    QUIT,
    OPTIONS,
    LEVEL_SELECT
};

struct MenuItem {
    std::string menuName;
    MenuItemId menuItemId;
    std::string text;
    int order;
    std::optional<std::string> subMenuName;
};

class Menu {
public:
    void addMenuItem(const std::string& menuName, MenuItem&& menuItem);
    std::vector<MenuItem> getMenuItems(const std::string& menuName);
    std::string getCurrentMenuName();
    int getCurrentMenuSize();
    void highlightNextItem();
    void highlightPreviousItem();
    MenuItemId selectCurrentItem();
    int getCurrentlyHighlightedItem();

private:
    std::vector<MenuItem> m_currentMenuItems; // convenience copy of current menu items
    std::unordered_multimap<std::string, MenuItem> m_menuItems;
    int m_currentlyHighlightedItem { 0 };
    std::string m_currentMenuName { "Main Menu" };
};

} // namespace amaze
} // namespace marengo