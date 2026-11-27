#pragma once

// Options / level selection dialog used by the Qt adapter. The dialog's
// result is QDialog::Accepted (OK), QDialog::Rejected (Cancel), or
// MenuDialog::ExitGame.

#include "controls.h"
#include "levelfilelister.h"

#include <QDialog>

#include <array>
#include <string>

class QLabel;
class QPushButton;
class QSlider;
class QTreeWidget;

namespace marengo {
namespace amaze {

class LevelProgress;

class MenuDialog final : public QDialog {
public:
    static constexpr int ExitGame = 2;

    MenuDialog(const std::string& dataDir, const MenuResults& initialValues, QWidget* parent);
    // Current values; these update immediately as the user changes them
    MenuResults results() const
    {
        return m_results;
    }
    // Dims, and stops the player choosing, levels they can't play yet, and
    // selects the current level
    void updateLevels(const LevelProgress& progress, const std::string& currentLevelFile);

protected:
    // Arrow-key navigation (also used by the gamepad, which sends arrow keys):
    // the dialog is a column of rows (the sliders, the level list, the
    // buttons). Up/Down moves between rows, leaving the level list only from
    // its first or last level; Left/Right adjusts a slider, moves between
    // buttons, or jumps from the level list to the row above / below.
    bool eventFilter(QObject* watched, QEvent* event) override;
    // Tab / Backtab cycle through the same rows. Qt's own order is replaced
    // because on macOS it skips sliders and buttons unless the system's
    // "Keyboard navigation" setting is on.
    bool focusNextPrevChild(bool next) override;

private:
    void updateFocusHighlight();
    // The first and last levels in the list which can be chosen
    int firstUnlockedRow() const;
    int lastUnlockedRow() const;
    // Top to bottom
    std::array<QSlider*, 3> sliders() const
    {
        return { m_musicVolume, m_deadZone, m_rumble };
    }

    MenuResults m_results;
    LevelFileLister m_levelFileLister;
    QTreeWidget* m_levelList;
    QSlider* m_musicVolume;
    QSlider* m_deadZone;
    QSlider* m_rumble;
    QLabel* m_musicVolumeLabel;
    QLabel* m_deadZoneLabel;
    QLabel* m_rumbleLabel;
    QPushButton* m_ok;
    QPushButton* m_cancel;
    QPushButton* m_exitGame;
};

} // namespace amaze
} // namespace marengo
