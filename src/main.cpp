#include "main.hpp"
#include "hooks.hpp"
#include "logging.hpp"

#include "GlobalNamespace/GameScenesManager.hpp"
#include "GlobalNamespace/HealthWarningFlowCoordinator.hpp"
#include "GlobalNamespace/HealthWarningFlowCoordinator_InitData.hpp"
#include "GlobalNamespace/MultiplayerLocalActivePlayerInstaller.hpp"
#include "GlobalNamespace/StandardGameplayInstaller.hpp"
#include "ReplaySystem/ReplayLoader.hpp"
#include "questui/shared/QuestUI.hpp"

#include "Data/Private/ReplayReader.hpp"
#include "MainInstaller.hpp"
#include "ReplaySystem/Installers/ImberInstaller.hpp"
#include "ReplaySystem/Installers/PlaybackInstaller.hpp"
#include "ReplaySystem/Installers/RecordInstaller.hpp"
#include "Services/FileService.hpp"
#include "UI/Other/Banner.hpp"
#include "Utils/TeamUtils.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSMLDataCache.hpp"
#include "lapiz/shared/zenject/Zenjector.hpp"
#include "assets.hpp"
#include "static.hpp"

#include "leaderboardcore/shared/LeaderboardCore.hpp"
#include "UI/Leaderboard/ScoreSaberCustomLeaderboard.hpp"
ModInfo modInfo = {MOD_ID, VERSION};

ScoreSaber::UI::Leaderboard::CustomLeaderboard leaderboard;
GlobalNamespace::IDifficultyBeatmap* currentDifficultyBeatmap;

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig()
{
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger()
{
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

// Called at the early stages of game loading
extern "C" __attribute((visibility("default"))) void setup(ModInfo& info)
{
    info = modInfo;

    getConfig().Load();
    getConfig().Reload();
    getConfig().Write();
    getLogger().info("Completed setup!");
}

// Called later on in the game loading - a good time to install function hooks
extern "C" __attribute((visibility("default"))) void load()
{
    il2cpp_functions::Init();
    // il2cpp_functions::Class_Init(classof(HMUI::ImageView*));
    // il2cpp_functions::Class_Init(classof(HMUI::CurvedTextMeshPro*));
    BSML::Init();
    QuestUI::Init();
    LeaderboardCore::Register::RegisterLeaderboard(&leaderboard, modInfo);
    custom_types::Register::AutoRegister();
    Hooks::InstallHooks(ScoreSaber::Logging::getLogger());
    TeamUtils::Download();
    ScoreSaber::Services::FileService::EnsurePaths();

    auto zenjector = Lapiz::Zenject::Zenjector::Get();
    zenjector->Install<ScoreSaber::MainInstaller*>(Lapiz::Zenject::Location::Menu);
    zenjector->Install<ScoreSaber::ReplaySystem::Installers::ImberInstaller*>(Lapiz::Zenject::Location::StandardPlayer);
    zenjector->Install<ScoreSaber::ReplaySystem::Installers::PlaybackInstaller*>(Lapiz::Zenject::Location::StandardPlayer);
    zenjector->Install<ScoreSaber::ReplaySystem::Installers::RecordInstaller*, GlobalNamespace::StandardGameplayInstaller*>();
    zenjector->Install<ScoreSaber::ReplaySystem::Installers::RecordInstaller*, GlobalNamespace::MultiplayerLocalActivePlayerInstaller*>();

    LeaderboardCore::Events::NotifyLeaderboardSet() += [](GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap){
        currentDifficultyBeatmap = difficultyBeatmap;
        auto vc = leaderboard.get_leaderboardViewController();
        if (vc && vc->isActivated) vc->onLeaderboardSet(difficultyBeatmap);
    };
}

BSML_DATACACHE(replay_png) {
    return IncludedAssets::replay_png;
}

BSML_DATACACHE(scoresaber_png) {
    return IncludedAssets::scoresaber_png;
}

BSML_DATACACHE(pixel_png) {
    return IncludedAssets::pixel_png;
}

BSML_DATACACHE(carat_png) {
    return IncludedAssets::carat_png;
}