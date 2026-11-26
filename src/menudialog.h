#pragma once

// Options / level selection dialog used by the Qt adapter. The dialog's
// result is QDialog::Accepted (OK), QDialog::Rejected (Cancel), or
// MenuDialog::ExitGame.

#include "controls.h"
#include "levelfilelister.h"

#include <QDialog>

#include <string>

class QLabel;
class QPushButton;
class QSlider;
class QTreeWidget;

namespace marengo {
namespace amaze {

class MenuDialog final : public QDialog {
public:
    static constexpr int ExitGame = 2;

    MenuDialog(const std::string& dataDir, const MenuResults& initialValues, QWidget* parent);
    // Current values; these update immediately as the user changes them
    MenuResults results() const
    {
        return m_results;
    }

protected:
    // Arrow-key navigation (also used by the gamepad, which sends arrow keys):
    // the dialog is a column of rows (the two sliders, the level list, the
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

    MenuResults m_results;
    LevelFileLister m_levelFileLister;
    QTreeWidget* m_levelList;
    QSlider* m_musicVolume;
    QSlider* m_deadZone;
    QLabel* m_musicVolumeLabel;
    QLabel* m_deadZoneLabel;
    QPushButton* m_ok;
    QPushButton* m_cancel;
    QPushButton* m_exitGame;
};

} // namespace amaze
} // namespace marengo
