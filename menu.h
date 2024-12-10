#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace marengo {
namespace amaze {

struct MenuItem {
    std::string itemName;
    int order;
    std::optional<std::vector<MenuItem>> subMenu;
};

class Menu {
public:
    void addMenuItem(const std::string& menuName, const MenuItem&& menuItem);
    std::vector<MenuItem> getMenuItems(const std::string& menuName);

private:
    std::unordered_multimap<std::string, MenuItem> m_menuItems;
};

} // namespace amaze
} // namespace marengo