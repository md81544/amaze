#pragma once

// Options / level selection dialog used by the Qt adapter. The dialog's
// result is QDialog::Accepted (OK), QDialog::Rejected (Cancel), or
// MenuDialog::ExitGame.

#include "igraphicsadapter.h"
#include "levelfilelister.h"

#include <QDialog>

#include <string>

class QListWidget;

namespace marengo {
namespace amaze {

class MenuDialog final : public QDialog {
public:
    static constexpr int ExitGame = 2;

    MenuDialog(const std::string& dataDir, QWidget* parent);
    // Current values; these update immediately as the user changes them
    MenuResults results() const
    {
        return m_results;
    }

private:
    MenuResults m_results;
    LevelFileLister m_levelFileLister;
    QListWidget* m_levelList;
};

} // namespace amaze
} // namespace marengo
