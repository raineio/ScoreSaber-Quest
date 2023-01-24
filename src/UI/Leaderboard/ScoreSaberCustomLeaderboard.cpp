#include "UI/Leaderboard/ScoreSaberCustomLeaderboard.hpp"
#include "UI/Leaderboard/ScoreSaberLeaderboardViewController.hpp"
#include "UI/Leaderboard/ScoreSaberPanel.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "bsml/shared/Helpers/creation.hpp"
#include "main.hpp"

namespace ScoreSaber::UI::Leaderboard
{
    ScoreSaberPanel* CustomLeaderboard::get_panelViewController()
    {
        if (!panelViewController || !panelViewController.isAlive())
        {
            panelViewController = BSML::Helpers::CreateViewController<ScoreSaber::UI::Leaderboard::ScoreSaberPanel*>();
        }
        return panelViewController.ptr();
    }

    ScoreSaberLeaderboardViewController* CustomLeaderboard::get_leaderboardViewController()
    {
        if (!leaderboardViewController || !leaderboardViewController.isAlive())
        {
            leaderboardViewController = BSML::Helpers::CreateViewController<ScoreSaber::UI::Leaderboard::ScoreSaberLeaderboardViewController*>();
        }
        return leaderboardViewController.ptr();
    }

    void CustomLeaderboard::deleteControllers(){
        leaderboardViewController = nullptr;
        panelViewController = nullptr;
    }
}