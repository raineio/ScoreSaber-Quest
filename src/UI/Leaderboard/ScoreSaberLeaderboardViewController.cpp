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
#include "bsml/shared/BSML/Components/ButtonIconImage.hpp"
#include "Polyglot/LocalizedTextMeshProUGUI.hpp"
#include "pinkcore/shared/RequirementAPI.hpp"

DEFINE_TYPE(ScoreSaber::UI::Leaderboard, ScoreSaberLeaderboardViewController);

extern ModInfo modInfo;

extern ScoreSaber::UI::Leaderboard::CustomLeaderboard leaderboard;
extern IDifficultyBeatmap* currentDifficultyBeatmap;
ScoreSaber::CustomTypes::Components::LeaderboardScoreInfoButtonHandler* leaderboardScoreInfoButtonHandler;
int _lastCell = 0;
int _leaderboardPage = 1;
bool _filterAroundCountry = false;
std::string _currentLeaderboardRefreshId;
int _lastScopeIndex = 0;

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

    void ScoreSaberLeaderboardViewController::PostParse(){
        Array<IconSegmentedControl::DataItem*>* array = ::Array<IconSegmentedControl::DataItem*>::New({
            IconSegmentedControl::DataItem::New_ctor(LoadSpriteRaw(IncludedAssets::Global_png), "Global"),
            IconSegmentedControl::DataItem::New_ctor(LoadSpriteRaw(IncludedAssets::Player_png), "Around You"),
            IconSegmentedControl::DataItem::New_ctor(LoadSpriteRaw(IncludedAssets::Friends_png), "Friends"),
            IconSegmentedControl::DataItem::New_ctor(LoadSpriteRaw(IncludedAssets::country_png), "Country"),
        });
        scopeSegmentedControl->SetData(array);
        CreateLoadingControl();
    }

    void ScoreSaberLeaderboardViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
    {
        getLogger().info("woo activated pog champing");
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
                    leaderboard.get_leaderboardViewController()->CheckPage();
                    leaderboard.get_leaderboardViewController()->onLeaderboardSet(currentDifficultyBeatmap);
                });
            });
        }
        else{
            leaderboard.get_leaderboardViewController()->CheckPage();
            leaderboard.get_leaderboardViewController()->onLeaderboardSet(currentDifficultyBeatmap);
        }
    }

    void ScoreSaberLeaderboardViewController::CheckPage()
    {
        auto* btn = up_button->get_transform()->GetComponentInChildren<UnityEngine::UI::Button*>();
        if (_leaderboardPage > 1) btn->set_interactable(true);
        else btn->set_interactable(false);
    }

    void ScoreSaberLeaderboardViewController::ChangeScope()
    {
        if (this->isActivated)
        {
            _leaderboardPage = 1;
            onLeaderboardSet(currentDifficultyBeatmap);
            CheckPage();
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
        if (state) PinkCore::RequirementAPI::EnablePlayButton(modInfo);
        else {
            // this is really icky since other mods rely on the pinkcore and the play button to see whether a level is playable or not
            // since scoresaber likes to disables this while the leaderboard loads, it causes some issues with other mods
            std::thread([](){
                std::this_thread::sleep_for(std::chrono::milliseconds(60));
                QuestUI::MainThreadScheduler::Schedule([](){
                    if (PlayerService::playerInfo.loginStatus == PlayerService::LoginStatus::Error) return;
                    PinkCore::RequirementAPI::DisablePlayButton(modInfo);
                });
            }).detach();
        }
    }

    void SetErrorState(LoadingControl* loadingControl, std::string errorText, bool showRefreshButton = false)
    {
        loadingControl->Hide();
        loadingControl->ShowText("literallyunalivemenow", false);
        loadingControl->ShowText(errorText, showRefreshButton);
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

    void LoadingControl_ShowLoading(LoadingControl* loadingControl){
        loadingControl->get_gameObject()->set_active(true);
        loadingControl->loadingContainer->SetActive(true);
        loadingControl->refreshContainer->SetActive(false);
        loadingControl->refreshText->SetText("");
    }

    void ScoreSaberLeaderboardViewController::OnIconSelected(IconSegmentedControl* segmentedControl, int index){
        _lastScopeIndex = index;
        ChangeScope();
    }

    void ScoreSaberLeaderboardViewController::OnPageUp(){
        _leaderboardPage--;
        onLeaderboardSet(currentDifficultyBeatmap);
        CheckPage();
    }

    void ScoreSaberLeaderboardViewController::OnPageDown(){
        _leaderboardPage++;
        onLeaderboardSet(currentDifficultyBeatmap);
        CheckPage();
    }

    void ScoreSaberLeaderboardViewController::onLeaderboardSet(IDifficultyBeatmap* difficultyBeatmap){
        SetPlayButtonState(true);
        auto* view = leaderboardTableView->get_transform()->GetComponentInChildren<LeaderboardTableView*>();
        auto* loadingControl = leaderboardTableView->GetComponent<LoadingControl*>();
        this->RefreshLeaderboard(difficultyBeatmap, view, _lastScopeIndex, loadingControl, System::Guid::NewGuid().ToString());
    }

    void ScoreSaberLeaderboardViewController::RefreshLeaderboard(IDifficultyBeatmap* difficultyBeatmap, LeaderboardTableView* tableView,
        PlatformLeaderboardsModel::ScoresScope scope, LoadingControl* loadingControl, std::string refreshId)
    {
        if (ScoreSaber::Services::UploadService::uploading) return;

        if (!this->isActivated) return;
        tableView->tableView->SetDataSource(nullptr, true);
        LoadingControl_ShowLoading(loadingControl);
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
            if (_currentLeaderboardRefreshId != refreshId) return;
            LeaderboardService::GetLeaderboardData(difficultyBeatmap, scope, _leaderboardPage,
                [=](ScoreSaber::Data::InternalLeaderboard internalLeaderboard) {
                    QuestUI::MainThreadScheduler::Schedule([=]() {
                        if (_currentLeaderboardRefreshId != refreshId) return;
                        if (internalLeaderboard.leaderboard.has_value())
                        {
                            int playerScoreIndex = GetPlayerScoreIndex(internalLeaderboard.leaderboard.value().scores);
                            if (internalLeaderboard.leaderboardItems->get_Count() != 0)
                            {                          
                                tableView->SetScores(internalLeaderboard.leaderboardItems, playerScoreIndex);
                                loadingControl->ShowText(System::String::_get_Empty(), false);
                                leaderboardScoreInfoButtonHandler->set_scoreCollection(internalLeaderboard.leaderboard.value().scores, internalLeaderboard.leaderboard->leaderboardInfo.id);
                                SetPlayButtonState(true);
                                SetRankedStatus(internalLeaderboard.leaderboard->leaderboardInfo);
                            }
                        }
                        else if (internalLeaderboard.leaderboardItems->size > 0)
                        {
                            SetErrorState(loadingControl, internalLeaderboard.leaderboardItems->items.get(0)->playerName);
                        }
                        else if (scope == PlatformLeaderboardsModel::ScoresScope::AroundPlayer)
                        {
                            SetErrorState(loadingControl, "You haven't set a score on this leaderboard");
                        }
                        else if (scope == PlatformLeaderboardsModel::ScoresScope::Friends)
                        {
                            SetErrorState(loadingControl, "You have no friends lmao");
                        }
                        else if (_leaderboardPage > 1)
                        {
                            SetErrorState(loadingControl, "No scores on this page");
                        }
                        else
                        {
                            SetErrorState(loadingControl, "No scores on this leaderboard, be the first!");
                        }
                    });
                }
            );
            
        });
        t.detach();
    }

    void ScoreSaberLeaderboardViewController::SetUploadState(bool state, bool success, std::string errorMessage)
    {
        QuestUI::MainThreadScheduler::Schedule([=]() {
            
            if (state)
            {
                LoadingControl_ShowLoading(leaderboardTableView->GetComponent<LoadingControl*>());
                leaderboard.get_panelViewController()->set_loading(true);
                leaderboard.get_panelViewController()->set_prompt("Uploading score...", -1);
                // ScoreSaberBanner->Prompt("Uploading Score", true, 5.0f, nullptr);
            }
            else
            {
                leaderboardTableView->GetComponent<LoadingControl*>()->refreshText->SetText("");
                onLeaderboardSet(currentDifficultyBeatmap);

                if (success)
                {
                    leaderboard.get_panelViewController()->set_prompt("<color=#89fc81>Score uploaded successfully</color>", 5);
                    PlayerService::UpdatePlayerInfo(true);
                }
                else
                {
                    leaderboard.get_panelViewController()->set_prompt(errorMessage, 5);
                }
            }
        });
    }

    void ScoreSaberLeaderboardViewController::CreateLoadingControl(){
        Object::Destroy(leaderboardTableView->GetComponent<LoadingControl*>());
        auto* loadingTemplate = UnityEngine::Resources::FindObjectsOfTypeAll<PlatformLeaderboardViewController*>()
            .FirstOrDefault()->get_transform()->Find("Container/LeaderboardTableView/LoadingControl")->GetComponentInChildren<LoadingControl*>();
        leaderboardTableView->AddComponent<LoadingControl*>()->Instantiate(loadingTemplate, leaderboardTableView->get_transform(), false);
        auto* loadingControl = leaderboardTableView->GetComponent<LoadingControl*>();
        Object::Destroy(loadingControl->get_transform()->GetComponentInChildren<TMPro::TextMeshProUGUI*>());
        loadingControl->loadingContainer = leaderboard_loading;
        loadingControl->downloadingContainer = leaderboard_loading;
        loadingControl->refreshContainer = Object::Instantiate(loadingTemplate->refreshContainer, leaderboardTableView->get_transform(), false);
        loadingControl->refreshContainer->get_transform()->set_localPosition(leaderboard_loading->get_transform()->get_localPosition());
        loadingControl->refreshText = loadingControl->get_transform()->GetComponentsInChildren<TMPro::TextMeshProUGUI*>().get(1);
        loadingControl->refreshButton = Object::Instantiate(loadingTemplate->refreshButton, leaderboardTableView->get_transform(), false);
        loadingControl->refreshButton->get_gameObject()->SetActive(false);
        loadingControl->refreshContainer->SetActive(false);
        GameObject::Destroy(loadingControl->refreshText->GetComponent<Polyglot::LocalizedTextMeshProUGUI*>());
    }

    void ScoreSaberLeaderboardViewController::AllowReplayWatching(bool value)
    {
        _allowReplayWatching = value;
    }

    bool ScoreSaberLeaderboardViewController::IsReplayWatchingAllowed()
    {
        return _allowReplayWatching;
    }
}
