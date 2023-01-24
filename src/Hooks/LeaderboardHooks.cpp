#include "Sprites.hpp"
#include "hooks.hpp"

// LeaderboardScoreUploader

#include "Data/Private/ReplayFile.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/LeaderboardScoreUploader_ScoreData.hpp"
#include "GlobalNamespace/PlatformLeaderboardsModel.hpp"
#include "ReplaySystem/ReplayLoader.hpp"

// StandardLevelScenesTransitionSetupDataSO

#include "GlobalNamespace/LevelCompletionResults.hpp"
#include "GlobalNamespace/MultiplayerLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/StandardLevelScenesTransitionSetupDataSO.hpp"

#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/LeaderboardTableView_ScoreData.hpp"
#include "GlobalNamespace/LoadingControl.hpp"
#include "GlobalNamespace/PlatformLeaderboardsModel.hpp"
#include "GlobalNamespace/PlatformLeaderboardsModel_LeaderboardScore.hpp"
#include "GlobalNamespace/PlatformLeaderboardsModel_ScoresScope.hpp"
#include "HMUI/SegmentedControl.hpp"
#include "Services/UploadService.hpp"
#include "System/Guid.hpp"
#include "Utils/BeatmapUtils.hpp"
#include "Utils/StringUtils.hpp"

#include "Data/Private/ReplayFile.hpp"
#include "ReplaySystem/Recorders/MainRecorder.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "GlobalNamespace/MenuTransitionsHelper.hpp"

#include "logging.hpp"
#include "UI/Leaderboard/ScoreSaberCustomLeaderboard.hpp"

using namespace HMUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace GlobalNamespace;
using namespace ScoreSaber;
using namespace ScoreSaber::UI::Other;
using namespace ScoreSaber::ReplaySystem;
using namespace ScoreSaber::Data::Private;

extern ScoreSaber::UI::Leaderboard::CustomLeaderboard leaderboard;

// Soft restart
MAKE_AUTO_HOOK_MATCH(MenuTransitionsHelper_RestartGame, &GlobalNamespace::MenuTransitionsHelper::RestartGame, void, GlobalNamespace::MenuTransitionsHelper* self, System::Action_1<Zenject::DiContainer*>* finishCallback)
{
    BeatmapUtils::playerDataModel = nullptr;
    leaderboard.deleteControllers(); // this does not work rip
    MenuTransitionsHelper_RestartGame(self, finishCallback);
}

MAKE_AUTO_HOOK_MATCH(StandardLevelScenesTransitionSetupDataSO_Finish, &GlobalNamespace::StandardLevelScenesTransitionSetupDataSO::Finish, void,
                     GlobalNamespace::StandardLevelScenesTransitionSetupDataSO* self,
                     GlobalNamespace::LevelCompletionResults* levelCompletionResults)
{
    ScoreSaber::Services::UploadService::Three(self, levelCompletionResults);
    StandardLevelScenesTransitionSetupDataSO_Finish(self, levelCompletionResults);
}

MAKE_AUTO_HOOK_MATCH(MultiplayerLevelScenesTransitionSetupDataSO_Finish, &GlobalNamespace::MultiplayerLevelScenesTransitionSetupDataSO::Finish, void,
                     GlobalNamespace::MultiplayerLevelScenesTransitionSetupDataSO* self,
                     GlobalNamespace::MultiplayerResultsData* multiplayerResultsData)
{
    ScoreSaber::Services::UploadService::Four(self, multiplayerResultsData);
    MultiplayerLevelScenesTransitionSetupDataSO_Finish(self, multiplayerResultsData);
}