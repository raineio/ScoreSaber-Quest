#include "UI/Leaderboard/ScoreSaberPanel.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "HMUI/Touchable.hpp"
#include "assets.hpp"
#include "bsml/shared/BSML.hpp"
#include "HMUI/ImageView.hpp"
#include "HMUI/CurvedCanvasSettingsHelper.hpp"

DEFINE_TYPE(ScoreSaber::UI::Leaderboard, ScoreSaberPanel);

namespace ScoreSaber::UI::Leaderboard
{

    void ScoreSaberPanel::set_status(std::string_view status, int scoreboardId)
    {
        leaderboard_ranked_text->SetText(string_format("<b><color=#FFDE1A>Ranked Status:</color></b> %s", status.data()));
        // this->scoreboardId = scoreboardId;
        // set_loading(false);
    }

    void ScoreSaberPanel::set_ranking(int rank, float pp)
    {
        global_rank_text->SetText(string_format("<b><color=#FFDE1A>Global Ranking: </color></b>#%d<size=3> (<color=#6772E5>%.2fpp</color></size>)", rank, pp));
        // set_loading(false);
    }

    void ScoreSaberPanel::PostParse(){
        HMUI::ImageView* bgImage = container->GetComponent<BSML::Backgroundable*>()->background;
        bgImage->skew = 0.18f;
        bgImage->gradient = true;
        bgImage->gradientDirection = 0;
        bgImage->set_color(defaultColor);
        scoresaber_logo->skew = 0.18f;
        separator->skew = 0.18f;
        
        // currently just here for POC
        set_ranking(1, 1234567890.0f);
        set_status("Deez Nuts", 1234567890);
    }

    void ScoreSaberPanel::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
    {
        if (!firstActivation) return;
        BSML::parse_and_construct(IncludedAssets::PanelView_bsml, this->get_transform(), this);
    }
}
