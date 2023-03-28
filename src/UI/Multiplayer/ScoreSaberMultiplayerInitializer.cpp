#include "UI/Multiplayer/ScoreSaberMultiplayerInitializer.hpp"

#include "Services/PlayerService.hpp"
#include "custom-types/shared/delegate.hpp"
#include <functional>
#include "UI/Leaderboard/ScoreSaberCustomLeaderboard.hpp"

DEFINE_TYPE(ScoreSaber::UI::Multiplayer, ScoreSaberMultiplayerInitializer);

extern ScoreSaber::UI::Leaderboard::CustomLeaderboard leaderboard;

namespace ScoreSaber::UI::Multiplayer
{
    void ScoreSaberMultiplayerInitializer::ctor(GameServerLobbyFlowCoordinator* gameServerLobbyFlowCoordinator)
    {
        _gameServerLobbyFlowCoordinator = gameServerLobbyFlowCoordinator;
    }

    void ScoreSaberMultiplayerInitializer::Initialize()
    {
        didSetupDelegate = custom_types::MakeDelegate<System::Action*>((std::function<void()>)[&]() { GameServerLobbyFlowCoordinator_didSetupEvent(); });
        didFinishDelegate = custom_types::MakeDelegate<System::Action*>((std::function<void()>)[&]() { GameServerLobbyFlowCoordinator_didFinishEvent(); });

        _gameServerLobbyFlowCoordinator->add_didSetupEvent(didSetupDelegate);
        _gameServerLobbyFlowCoordinator->add_didFinishEvent(didFinishDelegate);
    }

    void ScoreSaberMultiplayerInitializer::Dispose()
    {
        _gameServerLobbyFlowCoordinator->remove_didSetupEvent(didSetupDelegate);
        _gameServerLobbyFlowCoordinator->remove_didFinishEvent(didFinishDelegate);
    }

    void ScoreSaberMultiplayerInitializer::GameServerLobbyFlowCoordinator_didSetupEvent()
    {
        Services::PlayerService::AuthenticateUser([&](Services::PlayerService::LoginStatus loginStatus) {});
        leaderboard.get_leaderboardViewController()->AllowReplayWatching(false);
    }

    void ScoreSaberMultiplayerInitializer::GameServerLobbyFlowCoordinator_didFinishEvent()
    {
        leaderboard.get_leaderboardViewController()->AllowReplayWatching(true);
    }
} // namespace ScoreSaber::UI::Multiplayer