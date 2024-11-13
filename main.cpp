#include "sfmladapter.h"

#include "configreader.h"
#include "controller.h"
#include "exceptions.h"
#include "log.h"
#include "view.h"

#include <filesystem>
#include <print>

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

int main(int, char* argv[])
{
    using namespace marengo::amaze;

    try {

        std::filesystem::path exe(argv[0]);
        std::filesystem::path cwd = exe.remove_filename();

        INIT_MGOLOG((cwd / "debug.log").string());

        mgo::ConfigReader config((cwd / "amaze.cfg").string());

        // Until some form of menu is implemented, specify the level in the config
        // file. Not using commandline parameters because if I do, MacOS starts the
        // app without focus
        int gameLevel = config.readLong("GameLevel", 0);

        srand(static_cast<unsigned int>(time(NULL))); // TODO random device

        // Locate our data directory:
        std::string dataDir = (cwd / "data").string();

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
        View view(gameModel, graphicsManager);

        Controller controller(gameModel, view, graphicsManager);
        controller.mainLoop(gameLevel);

        // TODO model.preferencesSave();

        return 0;
    } catch (const marengo::amaze::AmazeBaseException& ex) {
        std::print(
            "Exception ({0}) thrown in {1} +{2} ({3}) : {4}\n",
            typeid(ex).name(),
            ex.file(),
            ex.line(),
            ex.function(),
            ex.what());
    } catch (const std::exception& e) {
        std::print("Exception encountered: {}\n", e.what());
    }
    return 1;
}
