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

int main(int argc, char* argv[])
{
    using namespace marengo::amaze;

    try {

        std::filesystem::path exe(argv[0]);
        std::filesystem::path cwd = exe.remove_filename();

        INIT_MGOLOG((cwd / "debug.log").string());

        mgo::ConfigReader config((cwd / "amaze.cfg").string());

        int gameLevel = 0;

        // For a reason I cannot find an answer to, MacOS starts our window with no focus
        // if a parameter (e.g. just the number 3) is supplied. If it sees a hyphen (i.e. flags)
        // then it doesn't. So rather than supporting flags fully, I just iterate over all
        // parameters and set the level required to the last parameter which can successfully be
        // converted to an int with stoi. This allows me to type "./amaze -- 3" (say) which
        // defeats MacOS's focus-losing behaviour. This doesn't seem to happen on older versions
        // of MacOS but has been replicated on two separate machines running Sonoma (MacOS 14.x).
        // This still doesn't seem to work if we specify full screen however :/
        // This is avoided by using an .app bundle and running that, it does mean it's then
        // harder to supply command-line arguments. TODO make the level selection in the game.
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
