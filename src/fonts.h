#pragma once

// The game's font, Oxanium, shared by the View and the menu dialog.
//
// It's a variable font, so its weight can be anything from 200 (ExtraLight)
// to 800 (ExtraBold), not just the named weights (400 Regular, 600 SemiBold,
// 700 Bold etc.). The weight is set on the font's "wght" axis, as
// QFont::setWeight() would round it to the nearest named weight.

#include <QFont>
#include <QFontDatabase>
#include <QString>

#include <string>

namespace marengo {
namespace amaze {
namespace fonts {

inline constexpr const char* FILE_NAME = "Oxanium-VariableFont_wght.ttf";
// Normal text
inline constexpr float WEIGHT = 600;
// Bold text (<b> or <strong>) in level introductions
inline constexpr float BOLD_WEIGHT = 800;

// The font at the given weight
inline QFont withWeight(QFont font, float weight)
{
    font.setVariableAxis(QFont::Tag("wght"), weight);
    return font;
}

// Loads the font (if it hasn't been already) and returns it at WEIGHT, or
// Qt's default font if it can't be loaded
inline QFont load(const std::string& dataDir, bool* loaded = nullptr)
{
    const int id
        = QFontDatabase::addApplicationFont(QString::fromStdString(dataDir + "/" + FILE_NAME));
    if (loaded) {
        *loaded = id >= 0;
    }
    if (id < 0) {
        return QFont();
    }
    return withWeight(QFont(QFontDatabase::applicationFontFamilies(id).at(0)), WEIGHT);
}

} // namespace fonts
} // namespace amaze
} // namespace marengo
