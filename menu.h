#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace marengo {
namespace amaze {

struct MenuItem {
    std::string itemName;
    std::string text;
    int order;
    std::optional<std::vector<MenuItem>> subMenu;
};

class Menu {
public:
    void addMenuItem(const std::string& menuName, const MenuItem&& menuItem);
    std::vector<MenuItem> getMenuItems(const std::string& menuName);
    std::string getCurrentMenuName();
    void highlightNextItem();
    void highlightPreviousItem();
    void selectCurrentItem();
    int getCurrentlyHighlightedItem();

private:
    std::unordered_multimap<std::string, MenuItem> m_menuItems;
    int m_currentlyHighlightedItem { 0 };
    std::string m_currentMenuName { "main" };
};

} // namespace amaze
} // namespace marengo