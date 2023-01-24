#pragma once

#include "leaderboardcore/shared/Models/CustomLeaderboard.hpp"
#include "UI/Leaderboard/ScoreSaberLeaderboardViewController.hpp"
#include "UI/Leaderboard/ScoreSaberPanel.hpp"

namespace ScoreSaber::UI::Leaderboard
{
    class CustomLeaderboard final : public LeaderboardCore::Models::CustomLeaderboard
    {
        public:
            ScoreSaberPanel* get_panelViewController() final override;
            ScoreSaberLeaderboardViewController* get_leaderboardViewController() final override;
            void deleteControllers();
        private:
            SafePtrUnity<ScoreSaberPanel> panelViewController;
            SafePtrUnity<ScoreSaberLeaderboardViewController> leaderboardViewController;
    };
}