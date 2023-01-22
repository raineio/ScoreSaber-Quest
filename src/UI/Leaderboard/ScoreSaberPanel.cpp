#include "UI/Leaderboard/ScoreSaberPanel.hpp"

#include "questui/shared/BeatSaberUI.hpp"
#include "HMUI/Touchable.hpp"
#include "assets.hpp"
#include "bsml/shared/BSML.hpp"

DEFINE_TYPE(ScoreSaber::UI::Leaderboard, ScoreSaberPanel);

namespace ScoreSaber::UI::Leaderboard
{

    bool ScoreSaberPanel::get_isClickableTop(){
        return _isClickableTop;
    }

    void ScoreSaberPanel::set_isClickableTop(bool value){
        _isClickableTop = true;
    }

    StringW ScoreSaberPanel::get_topText(){
        return _topText;
    }

    void ScoreSaberPanel::set_topText(StringW value){
        _topText = value;
    }

    void ScoreSaberPanel::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
    {
        if (firstActivation) {
            set_isClickableTop(true);
            set_topText("hey red this still doesn't work and i swear i followed your steps");
            BSML::parse_and_construct(IncludedAssets::basic_panel_double_text_bsml, this->get_transform(), this);
            
        }
    }
}
