#include "Services/UploadService.hpp"
#include "Data/InternalLeaderboard.hpp"
#include "Data/Private/ReplayFile.hpp"
#include "Data/Private/ScoreSaberUploadData.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapDifficultyMethods.hpp"
#include "GlobalNamespace/BeatmapDifficultySerializedMethods.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/PlatformLeaderboardsModel_ScoresScope.hpp"
#include "GlobalNamespace/PracticeViewController.hpp"
#include "ReplaySystem/Recorders/MainRecorder.hpp"
#include "ReplaySystem/ReplayLoader.hpp"
#include "Services/FileService.hpp"
#include "Services/LeaderboardService.hpp"
#include "Services/PlayerService.hpp"
#include "Services/ReplayService.hpp"
#include "UnityEngine/Application.hpp"
#include "UnityEngine/Resources.hpp"
#include "Utils/BeatmapUtils.hpp"
#include "Utils/StringUtils.hpp"
#include "Utils/WebUtils.hpp"
#include "Utils/md5.h"
#include "logging.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/QuestUI.hpp"
#include "static.hpp"
#include <chrono>
#include <thread>
#include "UI/Leaderboard/ScoreSaberCustomLeaderboard.hpp"

using namespace StringUtils;

using namespace GlobalNamespace;
using namespace ScoreSaber::Data::Private;
using namespace ScoreSaber::Static;
using namespace ScoreSaber::ReplaySystem;
using namespace ScoreSaber::Data::Private;

extern ScoreSaber::UI::Leaderboard::CustomLeaderboard leaderboard;


namespace ScoreSaber::Services::UploadService
{
    bool uploading;

    void Five(GlobalNamespace::StandardLevelScenesTransitionSetupDataSO* standardLevelScenesTransitionSetupData,
              GlobalNamespace::LevelCompletionResults* levelCompletionResults)
    {
        // return leaderboard.get_leaderboardViewController()->SetUploadState(false, false, "<color=#fc8181>not uploading scores on a test version silly</color>");

        if (StringUtils::GetEnv("disable_ss_upload") == "1")
        {
            return;
        }

        if (ScoreSaber::ReplaySystem::ReplayLoader::IsPlaying)
        {
            return;
        }

        PracticeViewController* practiceViewController = QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<PracticeViewController*>());
        if (practiceViewController->get_isInViewControllerHierarchy())
        {
            ReplayService::WriteSerializedReplay();
            return;
        }

        if (standardLevelScenesTransitionSetupData->gameMode == "Solo")
        {
            ReplayService::WriteSerializedReplay();
            if (standardLevelScenesTransitionSetupData->practiceSettings != nullptr)
                return;
            if (levelCompletionResults->levelEndAction != LevelCompletionResults::LevelEndAction::None)
                return;
            if (levelCompletionResults->levelEndStateType != LevelCompletionResults::LevelEndStateType::Cleared)
                return;

            // Continue to upload phase
            Six(standardLevelScenesTransitionSetupData->difficultyBeatmap, levelCompletionResults);
        }
    }

    void Six(GlobalNamespace::IDifficultyBeatmap* beatmap, GlobalNamespace::LevelCompletionResults* levelCompletionResults)
    {

        std::string encryptedPacket = CreateScorePacket(beatmap, levelCompletionResults->multipliedScore, levelCompletionResults->modifiedScore,
                                                        levelCompletionResults->fullCombo, levelCompletionResults->badCutsCount, levelCompletionResults->missedCount,
                                                        levelCompletionResults->maxCombo, levelCompletionResults->energy, levelCompletionResults->gameplayModifiers);
        auto previewBeatmapLevel = reinterpret_cast<IPreviewBeatmapLevel*>(beatmap->get_level());

        std::string levelHash = GetFormattedHash(previewBeatmapLevel->get_levelID());

        std::string characteristic = beatmap->get_parentDifficultyBeatmapSet()->get_beatmapCharacteristic()->serializedName;
        std::string songName = previewBeatmapLevel->get_songName();
        std::string difficultyName = BeatmapDifficultySerializedMethods::SerializedName(beatmap->get_difficulty());

        std::string replayFileName = ScoreSaber::Services::FileService::GetReplayFileName(levelHash, difficultyName, characteristic,
                                                                                          ScoreSaber::Services::PlayerService::playerInfo.localPlayerData.id, songName);
        Seven(beatmap, levelCompletionResults->modifiedScore, encryptedPacket, replayFileName);
    }

    void Seven(IDifficultyBeatmap* beatmap, int modifiedScore, std::string uploadPacket, std::string replayFileName)
    {
        std::thread t([beatmap, modifiedScore, uploadPacket, replayFileName] {
            leaderboard.get_leaderboardViewController()->SetUploadState(true, false);

            LeaderboardService::GetLeaderboardData(
                beatmap, PlatformLeaderboardsModel::ScoresScope::Global, 1, [=](Data::InternalLeaderboard internalLeaderboard) {
                    bool ranked = true;
                    if (internalLeaderboard.leaderboard.has_value())
                    {
                        ranked = internalLeaderboard.leaderboard.value().leaderboardInfo.ranked;
                        if (internalLeaderboard.leaderboard.value().leaderboardInfo.playerScore.has_value())
                        {
                            if (modifiedScore < internalLeaderboard.leaderboard.value().leaderboardInfo.playerScore.value().modifiedScore)
                            {
                                //  ERROR("Didn't beat score not uploading");
                                leaderboard.get_leaderboardViewController()->SetUploadState(false, false, "<color=#fc8181>Didn't beat score, not uploading</color>");
                                uploading = false;
                                return;
                            }
                        }
                    }
                    else
                    {
                        // ERROR("Failed to get leaderboards ranked status");
                    }

                    bool done = false;
                    bool failed = false;
                    int attempts = 0;

                    auto [beatmapDataBasicInfo, readonlyBeatmapData] = BeatmapUtils::getBeatmapData(beatmap);
                    bool containsV3Stuff = BeatmapUtils::containsV3Stuff(readonlyBeatmapData);

                    if (containsV3Stuff)
                    {
                        // ERROR("Beatmap contains V3 stuff, not uploading");
                        leaderboard.get_leaderboardViewController()->SetUploadState(false, false, "<color=#fc8181>New note type not supported, not uploading</color>");
                        uploading = false;
                        return;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
                    std::string url = BASE_URL + "/api/game/upload";
                    std::string postData = "data=" + uploadPacket;

                    while (!done)
                    {
                        uploading = true;
                        //  INFO("Uploading score...");
                        auto [responseCode, response] = WebUtils::PostWithReplaySync(url, ReplayService::CurrentSerializedReplay, uploadPacket, 30000);
                        if (responseCode == 200)
                        {
                            // INFO("Score uploaded successfully");
                            done = true;
                        }
                        if (responseCode == 403)
                        {
                            // INFO("Player banned, score didn't upload");
                            done = true;
                            failed = true;
                        }

                        if (!done)
                        {
                            if (attempts < 4)
                            {
                                // Failed but retry
                                // ERROR("Score failed to upload, retrying");
                                attempts++;
                                std::this_thread::sleep_for(2000ms);
                            }
                            else
                            {
                                done = true;
                                failed = true;
                            }
                        }

                    } // We out the loop now

                    if (done && !failed)
                    {
                        // Score uploaded successfully
                        // Save local replay
                        // INFO("Score uploaded");
                        SaveReplay(ReplayService::CurrentSerializedReplay, replayFileName);
                        leaderboard.get_leaderboardViewController()->SetUploadState(false, true);
                    }

                    if (failed)
                    {
                        // ERROR("Failed to upload score");
                        leaderboard.get_leaderboardViewController()->SetUploadState(false, false);
                        // Failed to upload score, tell user
                    }

                    uploading = false;
                }
            );
        });
        t.detach();
    }

    void SaveReplay(const std::vector<char>& replay, std::string replayFileName)
    {
        std::string filePath = ScoreSaber::Static::REPLAY_DIR + "/" + replayFileName + ".dat";
        std::ofstream file(filePath, ios::binary);
        file.write(replay.data(), replay.size());
    }

    std::string CreateScorePacket(GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap, int rawScore,
                                  int modifiedScore, bool fullCombo, int badCutsCount, int missedCount, int maxCombo, float energy,
                                  GlobalNamespace::GameplayModifiers* gameplayModifiers)
    {
        auto previewBeatmapLevel = reinterpret_cast<IPreviewBeatmapLevel*>(difficultyBeatmap->get_level());

        std::string levelHash = GetFormattedHash(previewBeatmapLevel->get_levelID());

        std::string gameMode = "Solo" + difficultyBeatmap->get_parentDifficultyBeatmapSet()->get_beatmapCharacteristic()->serializedName;
        int difficulty = BeatmapDifficultyMethods::DefaultRating(difficultyBeatmap->get_difficulty());

        std::string songName = previewBeatmapLevel->get_songName();
        std::string songSubName = previewBeatmapLevel->get_songSubName();
        std::string songAuthorName = previewBeatmapLevel->get_songAuthorName();
        std::string levelAuthorName = previewBeatmapLevel->get_levelAuthorName();
        int bpm = previewBeatmapLevel->get_beatsPerMinute();

        std::u16string playerName = ScoreSaber::Services::PlayerService::playerInfo.localPlayerData.name;
        std::string playerId = ScoreSaber::Services::PlayerService::playerInfo.localPlayerData.id;

        auto modifiers = GetModifierList(gameplayModifiers, energy);
        int hmd = 32;

        std::string infoHash = GetVersionHash();

        ScoreSaberUploadData data(playerName, playerId, rawScore, levelHash, songName, songSubName, levelAuthorName, songAuthorName, bpm,
                                  difficulty, infoHash, modifiers, gameMode, badCutsCount, missedCount, maxCombo, fullCombo, hmd);

        std::string uploadData = data.serialize();

        std::string key = md5("f0b4a81c9bd3ded1081b365f7628781f-" + ScoreSaber::Services::PlayerService::playerInfo.playerKey + "-" + playerId + "-f0b4a81c9bd3ded1081b365f7628781f");

        std::vector<unsigned char> keyBytes(key.begin(), key.end());
        std::vector<unsigned char> uploadDataBytes(uploadData.begin(), uploadData.end());
        std::vector<unsigned char> encrypted = Swap(uploadDataBytes, keyBytes);
        std::string result = ConvertToHex(encrypted);
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }

    std::vector<std::string> GetModifierList(GlobalNamespace::GameplayModifiers* gameplayModifiers, float energy)
    {
        std::vector<std::string> results;
        if (gameplayModifiers->energyType == GameplayModifiers::EnergyType::Battery)
        {
            results.push_back("BE");
        }
        if (gameplayModifiers->noFailOn0Energy && energy == 0)
        {
            results.push_back("NF");
        }
        if (gameplayModifiers->noFailOn0Energy && energy == -1)
        {
            results.push_back("NF");
        }
        if (gameplayModifiers->instaFail)
        {
            results.push_back("IF");
        }
        if (gameplayModifiers->failOnSaberClash)
        {
            results.push_back("SC");
        }
        if (gameplayModifiers->enabledObstacleType == GameplayModifiers::EnabledObstacleType::NoObstacles)
        {
            results.push_back("NO");
        }
        if (gameplayModifiers->noBombs)
        {
            results.push_back("NB");
        }
        if (gameplayModifiers->strictAngles)
        {
            results.push_back("SA");
        }
        if (gameplayModifiers->disappearingArrows)
        {
            results.push_back("DA");
        }
        if (gameplayModifiers->ghostNotes)
        {
            results.push_back("GN");
        }
        if (gameplayModifiers->songSpeed == GameplayModifiers::SongSpeed::Slower)
        {
            results.push_back("SS");
        }
        if (gameplayModifiers->songSpeed == GameplayModifiers::SongSpeed::Faster)
        {
            results.push_back("FS");
        }
        if (gameplayModifiers->songSpeed == GameplayModifiers::SongSpeed::SuperFast)
        {
            results.push_back("SF");
        }
        if (gameplayModifiers->smallCubes)
        {
            results.push_back("SC");
        }
        if (gameplayModifiers->strictAngles)
        {
            results.push_back("SA");
        }
        if (gameplayModifiers->proMode)
        {
            results.push_back("PM");
        }
        if (gameplayModifiers->noArrows)
        {
            results.push_back("NA");
        }
        return results;
    }

    std::vector<unsigned char> Swap(std::vector<unsigned char> panda1, std::vector<unsigned char> panda2)
    {
        int N1 = 11;
        int N2 = 13;
        int NS = 257;

        for (int i = 0; i <= panda2.size() - 1; i++)
        {
            NS += NS % (panda2[i] + 1);
        }

        std::vector<unsigned char> T(panda1.size());
        for (int i = 0; i <= panda1.size() - 1; i++)
        {
            NS = panda2[i % panda2.size()] + NS;
            N1 = (NS + 5) * (N1 & 255) + (N1 >> 8);
            N2 = (NS + 7) * (N2 & 255) + (N2 >> 8);
            NS = ((N1 << 8) + N2) & 255;

            T[i] = static_cast<unsigned char>(panda1[i] ^ static_cast<unsigned char>(NS));
        }

        return T;
    }

    std::string ConvertToHex(const std::vector<unsigned char>& v)
    {
        std::stringstream buffer;
        for (int i = 0; i < v.size(); i++)
        {
            buffer << std::hex << std::setfill('0');
            buffer << std::setw(2) << static_cast<unsigned>(v[i]);
        }
        return buffer.str();
    }

    std::string GetVersionHash()
    {
        return md5(std::string("Quest") + VERSION + (std::string)UnityEngine::Application::get_version());
    }
} // namespace ScoreSaber::Services::UploadService
