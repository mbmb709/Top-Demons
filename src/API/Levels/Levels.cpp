#include "Levels.hpp"
#include "../API.hpp"
#include "../../Utils/WedReq.hpp"
#include "../../Cache/Levels/Levels.hpp"
#include "../../Events/DemonlistLoadedEvent.hpp"
#include "../../Events/LevelLoadedEvent.hpp"

#include <algorithm>

namespace {
    constexpr std::size_t TOP_LIMIT = 150;
    constexpr std::size_t POINTERCRATE_PAGE_LIMIT = 100;

    void failDemonlist(APIError error) {
        DemonlistLoadedEvent().send(Err(error));
    }

    bool appendPointercratePage(
        matjson::Value const& data,
        std::vector<GDLLevel>& levels,
        int& lastPlacement
    ) {
        if (!data.isArray() || data.size() == 0) {
            return false;
        }

        for (auto const& demon : data) {
            int placement =
                demon["position"].asInt().unwrapOrDefault();
            if (
                placement < 1 ||
                placement > static_cast<int>(TOP_LIMIT)
            ) {
                continue;
            }

            int id = demon["id"].asInt().unwrapOrDefault();
            int ingameID =
                demon["level_id"].asInt().unwrapOrDefault();
            std::string name =
                demon["name"].asString().unwrapOrDefault();

            if (id <= 0 || ingameID <= 0 || name.empty()) {
                continue;
            }

            int listPercent =
                demon["requirement"].asInt().unwrapOrDefault();
            std::string holder =
                demon["publisher"]["name"]
                    .asString()
                    .unwrapOrDefault();
            std::string verifier =
                demon["verifier"]["name"]
                    .asString()
                    .unwrapOrDefault();
            int verifierID =
                demon["verifier"]["id"]
                    .asInt()
                    .unwrapOrDefault();
            std::string verificationURL =
                demon["video"].asString().unwrapOrDefault();

            levels.push_back(GDLLevel{
                id,
                ingameID,
                placement,
                std::move(name),
                0.0,
                listPercent,
                0,
                std::move(holder),
                std::move(verifier),
                verifierID,
                std::move(verificationURL),
                ""
            });

            lastPlacement = std::max(lastPlacement, placement);
        }

        return true;
    }

    bool isCompleteTop150(std::vector<GDLLevel>& levels) {
        std::ranges::sort(
            levels,
            {},
            [](GDLLevel const& level) {
                return level.placement;
            }
        );

        if (levels.size() != TOP_LIMIT) {
            return false;
        }

        for (std::size_t index = 0; index < levels.size(); index++) {
            if (
                levels[index].placement !=
                static_cast<int>(index + 1)
            ) {
                return false;
            }
        }

        return true;
    }

    void requestPointercratePage(
        std::vector<GDLLevel> levels,
        int after = 0
    ) {
        auto remaining = TOP_LIMIT - levels.size();
        auto limit = std::min(remaining, POINTERCRATE_PAGE_LIMIT);
        auto params = after > 0
            ? matjson::makeObject({
                {"limit", static_cast<int>(limit)},
                {"after", after}
            })
            : matjson::makeObject({
                {"limit", static_cast<int>(limit)}
            });

        Utils::WebReqRaw(
            GDL::API::POINTERCRATE_LIST_EP,
            params,
            [
                levels = std::move(levels),
                after
            ](matjson::Value data, APIError error) mutable {
                if (error) {
                    failDemonlist(error);
                    return;
                }

                auto previousSize = levels.size();
                int lastPlacement = after;
                if (
                    !appendPointercratePage(
                        data,
                        levels,
                        lastPlacement
                    ) ||
                    levels.size() == previousSize ||
                    lastPlacement <= after
                ) {
                    log::error(
                        "Top Demons received an invalid ranked-list page."
                    );
                    failDemonlist({
                        APIErrorType::InvalidEndpointResponse,
                        APIMessage::None
                    });
                    return;
                }

                if (levels.size() < TOP_LIMIT) {
                    requestPointercratePage(
                        std::move(levels),
                        lastPlacement
                    );
                    return;
                }

                if (!isCompleteTop150(levels)) {
                    log::error(
                        "Top Demons did not receive positions 1 through 150."
                    );
                    failDemonlist({
                        APIErrorType::InvalidEndpointResponse,
                        APIMessage::None
                    });
                    return;
                }

                GDL::Cache::Levels::setDemonlist(std::move(levels));
                DemonlistLoadedEvent().send(
                    Ok(GDL::Cache::Levels::getDemonlist())
                );
            }
        );
    }
}

namespace GDL::API::Levels {
    void getDemonlist() {
        auto& cachedDemonlist = GDL::Cache::Levels::getDemonlist();
        if (!cachedDemonlist.empty()) {
            DemonlistLoadedEvent().send(
                Ok(cachedDemonlist)
            );
            return;
        }

        std::vector<GDLLevel> levels;
        levels.reserve(TOP_LIMIT);
        requestPointercratePage(std::move(levels));
    }

    void getLevel(int levelID, bool isFullInfoRequire) {
        auto cachedLevel = GDL::Cache::Levels::getLevel(levelID);
        if (
            cachedLevel &&
            (isFullInfoRequire ? cachedLevel->isFull() : true)
        ) {
            LevelLoadedEvent(levelID).send(
                Ok(cachedLevel)
            );
            return;
        }

        Utils::WebReq(
            LEVEL_EP,
            matjson::makeObject({{"ingame_id", levelID}}),
            matjson::Value::object(),
            [levelID](matjson::Value data, APIError error) {
                if (error) {
                    LevelLoadedEvent(levelID).send(Err(error));
                    return;
                }
                if (!data.isObject() || data.size() == 0) {
                    log::error(
                        "Top Demons received invalid level information."
                    );
                    LevelLoadedEvent(levelID).send(
                        Err(APIError{
                            APIErrorType::InvalidEndpointResponse,
                            APIMessage::None
                        })
                    );
                    return;
                }

                int id = data["id"].asInt().unwrapOrDefault();
                int ingameID =
                    data["ingame_id"].asInt().unwrapOrDefault();
                int placement =
                    data["placement"].asInt().unwrapOrDefault();
                std::string name =
                    data["name"].asString().unwrapOrDefault();
                double points =
                    data["points"].asDouble().unwrapOrDefault();
                int listPercent =
                    data["list_percent"].asInt().unwrapOrDefault();
                int length =
                    data["length"].asInt().unwrapOrDefault();
                int objects =
                    data["objects"].asInt().unwrapOrDefault();
                std::string description =
                    data["description"].asString().unwrapOrDefault();
                std::string creator =
                    data["creator"].asString().unwrapOrDefault();
                std::string holder =
                    data["holder"].asString().unwrapOrDefault();
                std::string songURL =
                    data["song_url"].asString().unwrapOrDefault();
                int gameVersion =
                    data["game_version"].asInt().unwrapOrDefault();
                std::string verifier =
                    data["verification"]["username"]
                        .asString()
                        .unwrapOrDefault();
                int verifierID =
                    data["verification"]["user_id"]
                        .asInt()
                        .unwrapOrDefault();
                std::string verificationURL =
                    data["verification"]["video_url"]
                        .asString()
                        .unwrapOrDefault();
                bool isCopyable =
                    data["copy_info"]["is_copyable"]
                        .asBool()
                        .unwrapOrDefault();
                std::string password =
                    data["copy_info"]["password"]
                        .asString()
                        .unwrapOrDefault();
                std::string dateCreated =
                    data["date_created"].asString().unwrapOrDefault();

                if (
                    id <= 0 ||
                    ingameID <= 0 ||
                    placement < 1 ||
                    placement > static_cast<int>(TOP_LIMIT) ||
                    name.empty()
                ) {
                    LevelLoadedEvent(levelID).send(
                        Err(APIError{
                            APIErrorType::NoSearchResults,
                            APIMessage::LevelNotFound
                        })
                    );
                    return;
                }

                GDL::Cache::Levels::setLevel(GDLLevel{
                    id,
                    ingameID,
                    placement,
                    std::move(name),
                    points,
                    listPercent,
                    length,
                    std::move(holder),
                    std::move(verifier),
                    verifierID,
                    std::move(verificationURL),
                    std::move(dateCreated),
                    objects,
                    std::move(description),
                    std::move(creator),
                    std::move(songURL),
                    gameVersion,
                    isCopyable,
                    std::move(password)
                });
                LevelLoadedEvent(levelID).send(
                    Ok(GDL::Cache::Levels::getLevel(levelID))
                );
            }
        );
    }
}
