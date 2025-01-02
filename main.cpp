#include "sfmladapter.h"

#include "configreader.h"
#include "controller.h"
#include "exceptions.h"
#include "log.h"
#include "programoptions.h"
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

        srand(static_cast<unsigned int>(time(NULL))); // TODO random device

        // Locate our data directory:
        std::string dataDir = (cwd / "data").string();

        // The window's dimensions' proportions should mirror the physical screen's dimensions so
        // everything is in proportion if we go fullscreen
        int width = config.readLong("WindowWidth", 800);
        int height = config.readLong("WindowHeight", 500);
        bool useFullScreen = config.readBool("FullScreen", false);

        // Command-line options override config file settings:
        ProgramOptions programOptions(argc, argv);
        if (programOptions.cmdOptionExists("-h") || programOptions.cmdOptionExists("--help")) {
            std::print("Usage: amaze [-h] [-f] [-l (level FILE)] [level number]\n");
            return 0;
        }
        if (programOptions.cmdOptionExists("-f")
            || programOptions.cmdOptionExists("--fullscreen")) {
            useFullScreen = true;
        }
        int gameLevel = config.readLong("GameLevel", 0);
        if(programOptions.cmdOptionExists("-l")) {
            int level;
            try{
                level = std::atoi(programOptions.getCmdOption("-l").c_str());
                gameLevel = level;
            } catch(...) {
            }
        }
        // Specify a level FILE to load
        std::string levelFile;
        if (programOptions.cmdOptionExists("--file")) {
            levelFile = programOptions.getCmdOption("--file");
        }

        SfmlAdapter graphicsManager(width, height, useFullScreen, dataDir);

        GameModel gameModel(dataDir);
        View view(gameModel, graphicsManager);

        Controller controller(gameModel, view, graphicsManager);
        controller.mainLoop(gameLevel, levelFile);

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
