#include "preferences.h"
#include "sfmladapter.h"

#include "controller.h"
#include "exceptions.h"
#include "log.h"
#include "view.h"

#include <boost/filesystem.hpp>

#include <iostream>

// MVC intentions
// What goes where in the MVC pattern can be a matter for debate; here's
// what goes where in this particular application:
//
// MODEL
// Any state data should go in a model, obviously.
// Any logic which supports checking of state, modification of state etc
// can also go in a model. Note there are multiple models, all of which
// inherit from IModel. GameModel is the main one which contains the state of
// the game itself.
//
// VIEW
// Basically just code which interprets what it finds in a model and
// outputs it to the screen (with the aid of IGraphicsAdapter).
//
// CONTROLLER
// The "binding" between Model and View - this should contain logic
// related to the mechanics of the game BUT NOT logic related to actual
// gameplay (which should be in the model).

namespace {

std::string findDataDirectory(const char* const program)
{
    // Attempt to locate the data directory. We look in the
    // directory where the binary is located, its parent,
    // and its parent's parent. Throws if it cannot find a data directory.
    boost::filesystem::path searchDir(program);
    std::string dir { "" };
    searchDir = searchDir.parent_path();
    for (int n = 0; n < 3; ++n) {
        auto tryDir = searchDir;
        tryDir.append("data");
        if (boost::filesystem::is_directory(tryDir)) {
            auto tryFile = tryDir;
            tryFile.append("level0.cfg");
            if (boost::filesystem::exists(tryFile)) {
                dir = tryDir.string();
                break;
            }
        }
        searchDir = searchDir.parent_path();
    }
    if (dir.empty()) {
        THROWUP(marengo::amaze::AmazeStartupException, "Could not locate data directory");
    }
    return dir;
}
} // end anonymous namespace

int main(int argc, char* argv[])
{
    using namespace marengo::amaze;
    try {
        INIT_MGOLOG("debug.log");

        int gameLevel = 0;
        if (argc > 1) {
            gameLevel = std::stoi(argv[1]);
        }

        srand(static_cast<unsigned int>(time(NULL))); // TODO random device

        // Locate our data directory:
        std::string dataDir = findDataDirectory(argv[0]);

        int width = 800;
        int screenWidth = SfmlAdapter::getPhysicalScreenWidth();
        double screenProportion
            = width / static_cast<double>(SfmlAdapter::getPhysicalScreenWidth());
        int height = SfmlAdapter::getPhysicalScreenHeight() * screenProportion;
        // TODO: screen dimensions' proportions should mirror the physical
        // screen's dimensions so everything is in proportion if we go
        // fullscreen
        SfmlAdapter graphicsManager(width, static_cast<int>(height));

        GameModel gameModel;
        gameModel.setDataPath(dataDir);
        Preferences preferences(gameModel.getDataPath() + "amaze.cfg");
        View view(gameModel, graphicsManager);

        Controller controller(preferences, gameModel, view, graphicsManager);
        controller.mainLoop(gameLevel);

        // TODO model.preferencesSave();

        return 0;
    } catch (const marengo::amaze::AmazeBaseException& ex) {
        std::string message = "Exception (" + std::string(typeid(ex).name()) + ") thrown in "
            + ex.file() + " +" + std::to_string(ex.line()) + " (" + ex.function()
            + "): " + ex.what();
        std::cout << message << std::endl;
    } catch (const std::exception& e) {
        // anything caught here is a terminal event
        std::cout << e.what() << std::endl;
    }
    return 1;
}
