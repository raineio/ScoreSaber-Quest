#pragma once

#include "custom-types/shared/macros.hpp"
#include "HMUI/ViewController.hpp"
#include "bsml/shared/macros.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/UI/HorizontalLayoutGroup.hpp"
#include "bsml/shared/BSML/Components/Backgroundable.hpp"
#include "bsml/shared/BSML/Components/ClickableImage.hpp"
#include "bsml/shared/BSML/Components/ClickableText.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UI/Other/PlayerProfileModal.hpp"

DECLARE_CLASS_CODEGEN(ScoreSaber::UI::Leaderboard, ScoreSaberPanel, HMUI::ViewController,
    DECLARE_OVERRIDE_METHOD(void, DidActivate, il2cpp_utils::il2cpp_type_check::MetadataGetter<&HMUI::ViewController::DidActivate>::get(), bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::HorizontalLayoutGroup*, container);
    DECLARE_INSTANCE_FIELD(BSML::ClickableImage*, scoresaber_logo);
    DECLARE_INSTANCE_FIELD(HMUI::ImageView*, separator);
    DECLARE_INSTANCE_METHOD(void, PostParse);
    DECLARE_INSTANCE_FIELD(BSML::ClickableText*, global_rank_text);
    DECLARE_INSTANCE_FIELD(BSML::ClickableText*, leaderboard_ranked_text);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, promptText);
    DECLARE_INSTANCE_FIELD(UnityEngine::GameObject*, prompt_loader);
    DECLARE_INSTANCE_METHOD(void, OnLogoClick);
    DECLARE_INSTANCE_METHOD(void, OnRankTextClick);
    DECLARE_INSTANCE_FIELD(ScoreSaber::UI::Other::PlayerProfileModal*, playerProfileModal);

    public:
        void set_ranking(int rank, float pp);
        void set_status(std::string_view status, int scoreboardId);
        void Prompt(std::string status, bool loadingIndicator, float dismiss, std::function<void()> callback);
        void set_prompt(std::string text, int dismissTime);
        custom_types::Helpers::Coroutine SetPrompt(std::string status, bool showIndicator, float dismiss, std::function<void()> callback);
        void set_loading(bool value);
    private:
        static constexpr const UnityEngine::Color defaultColor = {0, 0.47, 0.72, 1.0};
)
