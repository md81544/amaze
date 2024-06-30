#include "preferences.h"
#include "sfmladapter.h"

#include "configreader.h"
#include "controller.h"
#include "exceptions.h"
#include "log.h"
#include "view.h"

#include <boost/filesystem.hpp>

#include <filesystem>
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

std::string findConfigFile(const char* const argv0)
{
    std::filesystem::path p(argv0);
    return p.string() + ".cfg";
}
std::string findDataDirectory(const char* const program)
{
    // TODO replace boost with std
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

        mgo::ConfigReader config(findConfigFile(argv[0]));

        int gameLevel = 0;

        // For a reason I cannot find an answer to, MacOS starts our window with no focus
        // if a parameter (e.g. just the number 3) is supplied. If it sees a hyphen (i.e. flags)
        // then it doesn't. So rather than supporting flags fully, I just iterate over all
        // parameters and set the level required to the last parameter which can successfully be
        // converted to an int with stoi. This allows me to type "./amaze -- 3" (say) which
        // defeats MacOS's focus-losing behaviour. This doesn't seem to happen on older versions
        // of MacOS but has been replicated on two separate machines running Sonoma (MacOS 14.x).
        // This still doesn't seem to work if we specify full screen however :/
        if (argc > 1) {
            for (int n = 1; n < argc; ++n) {
                try {
                    gameLevel = std::stoi(argv[n]);
                } catch (...) {
                }
            }
        }
        srand(static_cast<unsigned int>(time(NULL))); // TODO random device

        // Locate our data directory:
        std::string dataDir = findDataDirectory(argv[0]);

        // The window's dimensions' proportions should mirror the physical screen's dimensions so
        // everything is in proportion if we go fullscreen
        int width = config.readLong("WindowWidth", 800);
        int height = config.readLong("WindowHeight", 500);
        bool useFullScreen = config.readBool("FullScreen", false);
        if (useFullScreen) {
            // Check whether the SFML bug is going to hit us in full screen (see my bug report
            // https://github.com/SFML/SFML/issues/2300)
            if (!SfmlAdapter::isFullScreenSafe()) {
                useFullScreen = false;
            }
        }
        SfmlAdapter graphicsManager(width, height, useFullScreen);

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
