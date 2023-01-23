
#include "UI/Other/ScoreSaberLeaderboardView.hpp"

#include "CustomTypes/Components/LeaderboardScoreInfoButtonHandler.hpp"
#include "Data/InternalLeaderboard.hpp"
#include "Data/Score.hpp"

#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/LoadingControl.hpp"
#include "GlobalNamespace/PlatformLeaderboardViewController.hpp"
#include "GlobalNamespace/PlatformLeaderboardsHandler.hpp"
#include "GlobalNamespace/PlatformLeaderboardsModel.hpp"
#include "GlobalNamespace/PlatformLeaderboardsModel_GetScoresCompletionHandler.hpp"
#include "GlobalNamespace/PlatformLeaderboardsModel_GetScoresResult.hpp"
#include "GlobalNamespace/PlatformLeaderboardsModel_LeaderboardScore.hpp"
#include "GlobalNamespace/PlatformLeaderboardsModel_ScoresScope.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "Utils/BeatmapUtils.hpp"

#include "Services/UploadService.hpp"

#include "HMUI/CurvedCanvasSettingsHelper.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "HMUI/IconSegmentedControl.hpp"
#include "HMUI/IconSegmentedControl_DataItem.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/Screen.hpp"
#include "HMUI/ViewController_AnimationDirection.hpp"
#include "HMUI/ViewController_AnimationType.hpp"

#include "Services/LeaderboardService.hpp"
#include "Services/PlayerService.hpp"

#include "Sprites.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UI/FlowCoordinators/ScoreSaberFlowCoordinator.hpp"

#include "UI/Other/PanelView.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/SpriteRenderer.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "Utils/StringUtils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "hooks.hpp"
#include "logging.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "questui/shared/CustomTypes/Components/FloatingScreen/FloatingScreen.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "questui/shared/QuestUI.hpp"
#include <chrono>

#include "bsml/shared/BSML.hpp"
#include "assets.hpp"

using namespace HMUI;
using namespace QuestUI;
using namespace QuestUI::BeatSaberUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace GlobalNamespace;
using namespace ScoreSaber;
using namespace StringUtils;
using namespace ScoreSaber::CustomTypes;
using namespace ScoreSaber::UI::FlowCoordinators;
using namespace ScoreSaber::Services;

namespace ScoreSaber::UI::Other::ScoreSaberLeaderboardView
{
    ScoreSaber::UI::Other::Banner* ScoreSaberBanner;

    ScoreSaber::UI::Other::PanelView* view;
    ScoreSaber::CustomTypes::Components::LeaderboardScoreInfoButtonHandler* leaderboardScoreInfoButtonHandler;

    PlatformLeaderboardViewController* _platformLeaderboardViewController;

    UnityEngine::UI::Button* _pageUpButton;
    UnityEngine::UI::Button* _pageDownButton;

    bool _activated = false;

    int _lastCell = 0;
    int _leaderboardPage = 1;
    bool _filterAroundCountry = false;
    std::string _currentLeaderboardRefreshId;
    
    bool _allowReplayWatching = true;

    void ResetPage()
    {
        _leaderboardPage = 1;
    }

    void DidActivate(PlatformLeaderboardViewController* self, bool firstActivation, bool addedToHeirarchy,
                     bool screenSystemEnabling)
    {
        if (firstActivation)
        {
            // RedBrumbler top panel

            ScoreSaberBanner = ::ScoreSaber::UI::Other::Banner::Create(self->get_transform());
            auto playerProfileModal = ::ScoreSaber::UI::Other::PlayerProfileModal::Create(self->get_transform());
            ScoreSaberBanner->playerProfileModal = playerProfileModal;

            ScoreSaberBanner->Prompt("Signing into ScoreSaber...", false, 5.0f, nullptr);
            auto newGo = GameObject::New_ctor();
            auto t = newGo->get_transform();
            t->get_transform()->SetParent(self->get_transform(), false);
            t->set_localScale({1, 1, 1});
            leaderboardScoreInfoButtonHandler = newGo->AddComponent<ScoreSaber::CustomTypes::Components::LeaderboardScoreInfoButtonHandler*>();
            leaderboardScoreInfoButtonHandler->Setup();
            leaderboardScoreInfoButtonHandler->scoreInfoModal->playerProfileModal = playerProfileModal;
            leaderboardScoreInfoButtonHandler = leaderboardScoreInfoButtonHandler;
            leaderboardScoreInfoButtonHandler->set_buttonCount(0);

            _activated = true;
        }
    }

    void DidDeactivate()
    {
        _activated = false;
        _pageUpButton = nullptr;
        _pageDownButton = nullptr;
    }

    void SetPlayButtonState(bool state)
    {
        //add pinkcore stuff again
    }

    void SetUploadState(bool state, bool success, std::string errorMessage)
    {
        QuestUI::MainThreadScheduler::Schedule([=]() {
            if (state)
            {
                _platformLeaderboardViewController->loadingControl->ShowLoading(System::String::_get_Empty());
                ScoreSaberBanner->set_loading(true);
                ScoreSaberBanner->set_prompt("Uploading score...", -1);
                // ScoreSaberBanner->Prompt("Uploading Score", true, 5.0f, nullptr);
            }
            else
            {
                _platformLeaderboardViewController->Refresh(true, true);

                if (success)
                {
                    ScoreSaberBanner->set_prompt("<color=#89fc81>Score uploaded successfully</color>", 5);
                    PlayerService::UpdatePlayerInfo(true);
                }
                else
                {
                    ScoreSaberBanner->set_prompt(errorMessage, 5);
                }
            }
        });
    }

    void AllowReplayWatching(bool value)
    {
        _allowReplayWatching = value;
    }

    bool IsReplayWatchingAllowed()
    {
        return _allowReplayWatching;
    }

} // namespace ScoreSaber::UI::Other::ScoreSaberLeaderboardView
