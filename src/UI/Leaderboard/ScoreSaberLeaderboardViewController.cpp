#include "UI/Leaderboard/ScoreSaberLeaderboardViewController.hpp"
#include "leaderboardcore/shared/LeaderboardCore.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "bsml/shared/BSML.hpp"
#include "assets.hpp"
#include "Services/PlayerService.hpp"
#include "UI/Leaderboard/ScoreSaberCustomLeaderboard.hpp"
#include "GlobalNamespace/LoadingControl.hpp"
#include "bsml/shared/Helpers/utilities.hpp"
#include "main.hpp"
#include "CustomTypes/Components/LeaderboardScoreInfoButtonHandler.hpp"
#include "Services/UploadService.hpp"
#include "Services/LeaderboardService.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include <functional>
#include "System/Guid.hpp"

DEFINE_TYPE(ScoreSaber::UI::Leaderboard, ScoreSaberLeaderboardViewController);

extern ScoreSaber::UI::Leaderboard::CustomLeaderboard leaderboard;
extern IDifficultyBeatmap* currentDifficultyBeatmap;
ScoreSaber::CustomTypes::Components::LeaderboardScoreInfoButtonHandler* leaderboardScoreInfoButtonHandler;
int _lastCell = 0;
int _leaderboardPage = 1;
bool _filterAroundCountry = false;
std::string _currentLeaderboardRefreshId;

using namespace QuestUI;
using namespace QuestUI::BeatSaberUI;
using namespace HMUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace ScoreSaber::Services;
using namespace BSML::Utilities;
using namespace HMUI;
using namespace GlobalNamespace;

namespace ScoreSaber::UI::Leaderboard
{

    void ScoreSaberLeaderboardViewController::OnIconSelected(IconSegmentedControl* segmentedControl, int index){
        getLogger().info("I'm pogging my pants at index %i", index);
    }

    void ScoreSaberLeaderboardViewController::PostParse(){
        Array<IconSegmentedControl::DataItem*>* array = ::Array<IconSegmentedControl::DataItem*>::New({
            IconSegmentedControl::DataItem::New_ctor(LoadSpriteRaw(IncludedAssets::Global_png), "Global"),
            IconSegmentedControl::DataItem::New_ctor(LoadSpriteRaw(IncludedAssets::Player_png), "Around You"),
            IconSegmentedControl::DataItem::New_ctor(LoadSpriteRaw(IncludedAssets::Friends_png), "Friends"),
            IconSegmentedControl::DataItem::New_ctor(LoadSpriteRaw(IncludedAssets::country_png), "Country"),
        });
        scopeSegmentedControl->SetData(array);
    }

    void ScoreSaberLeaderboardViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
    {
        if (firstActivation)
        {

            BSML::parse_and_construct(IncludedAssets::ScoreSaberLeaderboardViewController_bsml, get_transform(), this);
            leaderboard.get_panelViewController()->Prompt("Signing into ScoreSaber...", false, 5.0f, nullptr);

            auto playerProfileModal = ::ScoreSaber::UI::Other::PlayerProfileModal::Create(get_transform());
            auto newGo = GameObject::New_ctor();
            auto t = newGo->get_transform();
            t->get_transform()->SetParent(get_transform(), false);
            t->set_localScale({1, 1, 1});
            leaderboardScoreInfoButtonHandler = newGo->AddComponent<ScoreSaber::CustomTypes::Components::LeaderboardScoreInfoButtonHandler*>();
            leaderboardScoreInfoButtonHandler->Setup();
            leaderboardScoreInfoButtonHandler->scoreInfoModal->playerProfileModal = playerProfileModal;
            leaderboardScoreInfoButtonHandler = leaderboardScoreInfoButtonHandler;
            leaderboardScoreInfoButtonHandler->set_buttonCount(0);

            leaderboard.get_panelViewController()->playerProfileModal = playerProfileModal;

            PlayerService::AuthenticateUser([&](PlayerService::LoginStatus loginStatus) {
                switch (loginStatus)
                {
                    case PlayerService::LoginStatus::Success: {
                        leaderboard.get_panelViewController()->Prompt("<color=#89fc81>Successfully signed in to ScoreSaber</color>", false, 5.0f, nullptr);
                        break;
                    }
                    case PlayerService::LoginStatus::Error: {
                        leaderboard.get_panelViewController()->Prompt("<color=#fc8181>Authentication failed</color>", false, 5.0f, nullptr);
                        break;
                    }
                }
                QuestUI::MainThreadScheduler::Schedule([]() {
                    leaderboard.get_leaderboardViewController()->onLeaderboardSet(currentDifficultyBeatmap);
                });
            });
        }
    }

    int GetPlayerScoreIndex(std::vector<Data::Score> scores)
    {
        for (int i = 0; i < scores.size(); i++)
        {
            if (scores[i].leaderboardPlayerInfo.id == PlayerService::playerInfo.localPlayerData.id)
            {
                return i;
            }
        }
        return -1;
    }

    void SetPlayButtonState(bool state)
    {
        //add pinkcore stuff again
    }

    void SetErrorState(LoadingControl* loadingControl, std::string errorText, bool showRefreshButton = true)
    {
        // loadingControl->Hide();
        // loadingControl->ShowText(errorText, showRefreshButton);
        SetPlayButtonState(true);
    }

    void SetRankedStatus(Data::LeaderboardInfo leaderboardInfo)
    {
        if (leaderboardInfo.ranked)
        {
            if (leaderboardInfo.positiveModifiers)
            {
                leaderboard.get_panelViewController()->set_status("Ranked (DA = +0.02, GN +0.04)", leaderboardInfo.id);
            }
            else
            {
                leaderboard.get_panelViewController()->set_status("Ranked (modifiers disabled)", leaderboardInfo.id);
            }
            return;
        }

        if (leaderboardInfo.qualified)
        {
            leaderboard.get_panelViewController()->set_status("Qualified", leaderboardInfo.id);
            return;
        }

        if (leaderboardInfo.loved)
        {
            leaderboard.get_panelViewController()->set_status("Loved", leaderboardInfo.id);
            return;
        }
        leaderboard.get_panelViewController()->set_status("Unranked", leaderboardInfo.id);
    }

    void ScoreSaberLeaderboardViewController::onLeaderboardSet(IDifficultyBeatmap* difficultyBeatmap){
        auto* view = leaderboardTableView->get_transform()->GetComponentInChildren<LeaderboardTableView*>();
;
        this->RefreshLeaderboard(difficultyBeatmap, view, 0, nullptr, System::Guid::NewGuid().ToString());
    }

    void ScoreSaberLeaderboardViewController::RefreshLeaderboard(IDifficultyBeatmap* difficultyBeatmap, LeaderboardTableView* tableView,
        PlatformLeaderboardsModel::ScoresScope scope, LoadingControl* loadingControl, std::string refreshId)
    {
        if (ScoreSaber::Services::UploadService::uploading) return;

        if (!this->isActivated) return;
        tableView->tableView->SetDataSource(nullptr, true);
        leaderboard_loading->SetActive(true);
        SetPlayButtonState(false);
        leaderboardScoreInfoButtonHandler->set_buttonCount(0);

        if (PlayerService::playerInfo.loginStatus == PlayerService::LoginStatus::Error)
        {
            SetErrorState(loadingControl, "ScoreSaber authentication failed, please restart Beat Saber", false);
            SetPlayButtonState(true);
            return;
        }

        if (PlayerService::playerInfo.loginStatus != PlayerService::LoginStatus::Success) return;

        _currentLeaderboardRefreshId = refreshId;

        std::thread t([difficultyBeatmap, scope, loadingControl, tableView, refreshId, this] {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (_currentLeaderboardRefreshId == refreshId)
            {
                LeaderboardService::GetLeaderboardData(
                    difficultyBeatmap, scope, _leaderboardPage,
                    [=](ScoreSaber::Data::InternalLeaderboard internalLeaderboard) {
                        QuestUI::MainThreadScheduler::Schedule([=]() {
                            if (internalLeaderboard.leaderboard.has_value())
                            {
                                int playerScoreIndex = GetPlayerScoreIndex(internalLeaderboard.leaderboard.value().scores);
                                if (internalLeaderboard.leaderboardItems->get_Count() != 0)
                                {
                                    if (scope == PlatformLeaderboardsModel::ScoresScope::AroundPlayer && playerScoreIndex == -1)
                                    {
                                        SetErrorState(loadingControl, "You haven't set a score on this leaderboard", true);
                                    }
                                    else
                                    {
                                        tableView->SetScores(internalLeaderboard.leaderboardItems, playerScoreIndex);
                                        // loadingControl->ShowText(System::String::_get_Empty(), false);
                                        // loadingControl->Hide();
                                        leaderboard_loading->set_active(false);
                                        leaderboardScoreInfoButtonHandler->set_scoreCollection(internalLeaderboard.leaderboard.value().scores, internalLeaderboard.leaderboard->leaderboardInfo.id);
                                        SetPlayButtonState(true);
                                        SetRankedStatus(internalLeaderboard.leaderboard->leaderboardInfo);
                                    }
                                }
                                else
                                {
                                    if (_leaderboardPage > 1)
                                    {
                                        SetErrorState(loadingControl, "No scores on this page");
                                    }
                                    else
                                    {
                                        SetErrorState(loadingControl, "No scores on this leaderboard, be the first!");
                                    }
                                }
                            }
                            else
                            {
                                if (internalLeaderboard.leaderboardItems->get_Item(0) != nullptr)
                                {
                                    SetErrorState(loadingControl, internalLeaderboard.leaderboardItems->get_Item(0)->get_playerName(), false);
                                }
                                else
                                {
                                    SetErrorState(loadingControl, "No scores on this leaderboard, be the first! 0x1");
                                }
                            }
                        });
                    },
                    _filterAroundCountry);
            }
        });
        t.detach();
    }
}
