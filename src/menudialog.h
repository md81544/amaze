#pragma once

// Options / level selection dialog used by the Qt adapter. The dialog's
// result is QDialog::Accepted (OK), QDialog::Rejected (Cancel), or
// MenuDialog::ExitGame.
//
// It has two tabs, Levels and Options, above OK / Cancel / Exit Game. It's
// styled like the in-game ship status panel (see palette.h).

#include "controls.h"
#include "levelfilelister.h"

#include <QDialog>

#include <optional>
#include <string>
#include <utility>
#include <vector>

class QFrame;
class QLabel;
class QPushButton;
class QSlider;
class QStackedWidget;
class QTabBar;
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
    // Call before showing the dialog: it opens on the Levels tab, with the
    // display option showing whether the game is full screen
    void prepareToShow(bool fullScreen);
    // The names of the gamepad's shoulder buttons, which switch tabs, to show
    // as a hint; none if no gamepad is attached
    void setShoulderButtonNames(const std::optional<std::pair<std::string, std::string>>& names);
    // Moves to the tab on the left (-1) or right (1), if there is one
    void switchTab(int direction);

protected:
    // Arrow-key navigation (also used by the gamepad, which sends arrow keys).
    // Each tab is a column of rows above the buttons: the level list, or the
    // options. Up/Down moves between rows, and from the last row down to the
    // buttons; Left/Right adjusts an option, moves between buttons, or jumps
    // from the level list to the buttons. Ctrl+Tab / Ctrl+Shift+Tab switch
    // tabs.
    bool eventFilter(QObject* watched, QEvent* event) override;
    // Tab / Backtab cycle through the current tab's rows and the buttons.
    // Qt's own order is replaced because on macOS it skips sliders and
    // buttons unless the system's "Keyboard navigation" setting is on.
    bool focusNextPrevChild(bool next) override;

private:
    enum Tab {
        LevelsTab,
        OptionsTab,
    };
    // An option: its label (lit up when it has focus) and control
    struct Option {
        QLabel* label;
        QWidget* control;
    };

    QWidget* makeLevelsPage();
    QWidget* makeOptionsPage();
    void setFullScreenChoice(bool fullScreen);
    void showTab(int index);
    // The rows of the current tab, top to bottom
    std::vector<QWidget*> rows() const;
    void updateFocusHighlight();
    // The first and last levels in the list which can be chosen
    int firstUnlockedRow() const;
    int lastUnlockedRow() const;

    MenuResults m_results;
    LevelFileLister m_levelFileLister;
    QTabBar* m_tabs;
    QStackedWidget* m_pages;
    QLabel* m_leftShoulderHint;
    QLabel* m_rightShoulderHint;
    std::optional<std::pair<std::string, std::string>> m_shoulderButtonNames;
    QTreeWidget* m_levelList;
    std::vector<Option> m_options;
    QFrame* m_displayChoice { nullptr };
    QPushButton* m_fullScreen { nullptr };
    QPushButton* m_windowed { nullptr };
    // The option which last had focus, to return to from the buttons
    QWidget* m_lastOption { nullptr };
    QPushButton* m_ok;
    QPushButton* m_cancel;
    QPushButton* m_exitGame;
};

} // namespace amaze
} // namespace marengo
