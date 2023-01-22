#include "UI/Leaderboard/ScoreSaberCustomLeaderboard.hpp"
#include "UI/Leaderboard/ScoreSaberLeaderboardViewController.hpp"
#include "UI/Leaderboard/ScoreSaberPanel.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "bsml/shared/Helpers/creation.hpp"

namespace ScoreSaber::UI::Leaderboard
{
    HMUI::ViewController* CustomLeaderboard::get_panelViewController()
    {
        if (!panelViewController)
        {
            panelViewController = BSML::Helpers::CreateViewController<ScoreSaber::UI::Leaderboard::ScoreSaberPanel*>();
        }
        return panelViewController.ptr();
    }

    HMUI::ViewController* CustomLeaderboard::get_leaderboardViewController()
    {
        if (!leaderboardViewController)
        {
            leaderboardViewController = BSML::Helpers::CreateViewController<ScoreSaber::UI::Leaderboard::ScoreSaberLeaderboardViewController*>();
        }
        return leaderboardViewController.ptr();
    }
}