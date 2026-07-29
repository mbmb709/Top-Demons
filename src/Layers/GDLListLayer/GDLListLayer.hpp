#pragma once

#include "../../Utils/CalculateCoverScale.hpp"
#include "../../Settings/Settings.hpp"

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

using namespace geode::prelude;

class GDLListLayer : public CCLayer, SetIDPopupDelegate, LevelManagerDelegate {
public:
	static GDLListLayer* create();
	static CCScene* scene();

	void onBack();
	void keyDown(enumKeyCodes, double delta) override;
	void keyBackClicked() override;
    virtual void onExit() override;

protected:
	enum class ValidationStage {
		None,
		LevelIDs,
		ReplacementSearch
	};

	struct InvalidLevel {
		size_t index;
		int oldLevelID;
		std::string name;
	};

	static constexpr size_t VALIDATION_BATCH_SIZE = 100;

	ListenerHandle m_demonlistLoadListener;
	ListenerHandle m_populateListListener;
	std::vector<int> m_gdlLevels;
	std::vector<int> m_levelsToValidate;
	std::vector<int> m_resolvedLevelIDs;
	std::vector<InvalidLevel> m_invalidLevels;
	std::unordered_set<int> m_acceptedLevelIDs;

	ValidationStage m_validationStage = ValidationStage::None;
	size_t m_validationBatch = 0;
	size_t m_invalidLevelIndex = 0;
	int m_validationRetryCount = 0;
	int m_replacementSearchPage = 0;

	GJListLayer* m_levelList;
	CCLabelBMFont* m_errorMessage;
	LoadingSpinner* m_loadingSpinner;

	CCNode* m_searchBarMenu;
	TextInput* m_searchBar;

	CCLabelBMFont* m_levelsCountLabel;
	CCLabelBMFont* m_pageLabel;
    
	CCMenu* m_pageMenu;
	InfoAlertButton* m_infoBtn;
	CCMenuItemSpriteExtra* m_backBtn;
	CCMenuItemSpriteExtra* m_pageBtn;
	CCMenuItemSpriteExtra* m_leftBtn;
	CCMenuItemSpriteExtra* m_rightBtn;
	CCMenuItemSpriteExtra* m_randomBtn;
	CCMenuItemSpriteExtra* m_firstBtn;
	CCMenuItemSpriteExtra* m_lastBtn;
    
	int m_page = 1;
	int m_lvlsPerPage = Settings::increaseLevelsPerPage() ? 25 : 10;
	std::string m_query;
	std::vector<int> m_searchResults;

	bool init() override;
	void startLevelValidation();
	void loadValidationBatch(float);
	void processValidationBatch(CCArray* levels);
	void searchNextReplacement(float);
	void processReplacementSearch(CCArray* levels);
	void finishLevelValidation();
	void skipCurrentInvalidLevel();
	void updateValidationStatus();
	bool isRatedExtremeDemon(GJGameLevel* level) const;
	static std::string normalizeLevelName(std::string_view name);
	void cacheGameLevel(GJGameLevel* level);
	void populateList();
	std::vector<int> getSuitableLevels();
	void search();
	void page(int page);
	void setupPageInfo(gd::string, const char*) override;
	void showLoading();
	void loadLevelsFinished(CCArray* levels, const char* key, int) override;
	void loadLevelsFailed(char const* key) override;
	void loadLevelsFailed(char const* key, int) override;
	void setIDPopupClosed(SetIDPopup*, int) override;
	
	int calculateMaxPage();
};
