#include "Levels.hpp"
#include "../Cache.hpp"

#include <algorithm>
#include <optional>
#include <unordered_map>

namespace GDL::Cache::Levels {
    constexpr auto LEVELS_TTL = std::chrono::minutes{30};

    static std::vector<int> levelList;
    static std::unordered_map<int, CacheEntry<GDLLevel>> levelData;
    static std::vector<int> levelsWOPlacement;
    static std::optional<std::chrono::steady_clock::time_point>
        demonlistCachedAt;

    const std::vector<int>& getDemonlist() {
        if (
            demonlistCachedAt.has_value() &&
            isExpired(demonlistCachedAt.value(), LEVELS_TTL)
        ) {
            clear();
        }

        return levelList;
    }

    void setDemonlist(std::vector<GDLLevel>&& levels) {
        clear();
        auto now = std::chrono::steady_clock::now();

        for (auto& level : levels) {
            int levelID = level.ingameID;
            levelData.insert_or_assign(
                levelID,
                CacheEntry{std::move(level), now}
            );
            levelList.push_back(levelID);
        }

        demonlistCachedAt = now;
    }


    const GDLLevel* getLevel(int levelID) {
        auto it = levelData.find(levelID);
        if (
            it == levelData.end() ||
            isExpired(it->second.cachedAt, LEVELS_TTL)
        ) return nullptr;

        return &it->second.value;
    }
    
    void setLevel(GDLLevel&& level) {
        int levelID = level.ingameID;
        levelData.insert_or_assign(
            levelID,
            CacheEntry{std::move(level), std::chrono::steady_clock::now()}
        );

        if (
            std::ranges::find(levelList, levelID) ==
            levelList.end()
        ) {
            levelList.push_back(levelID);
        }
    }

    bool replaceLevelID(int oldLevelID, int newLevelID) {
        if (oldLevelID <= 0 || newLevelID <= 0) return false;
        if (oldLevelID == newLevelID) return true;
        if (levelData.contains(newLevelID)) return false;

        auto oldLevel = levelData.find(oldLevelID);
        if (oldLevel == levelData.end()) return false;

        auto entry = std::move(oldLevel->second);
        levelData.erase(oldLevel);

        entry.value.ingameID = newLevelID;
        entry.cachedAt = std::chrono::steady_clock::now();
        levelData.insert_or_assign(newLevelID, std::move(entry));

        std::ranges::replace(levelList, oldLevelID, newLevelID);
        return true;
    }

    void removeLevel(int levelID) {
        levelData.erase(levelID);
        std::erase(levelList, levelID);
    }

    
    bool isLevelWOPlacement(int levelID) {
        auto it = std::find(levelsWOPlacement.begin(), levelsWOPlacement.end(), levelID);
        return it != levelsWOPlacement.end();
    }

    void setLevelWOPlacement(int levelID) {
        levelsWOPlacement.push_back(levelID);
    }
    

    void clear() {
        levelData.clear();
        levelList.clear();
        levelsWOPlacement.clear();
        demonlistCachedAt.reset();
    }
};
