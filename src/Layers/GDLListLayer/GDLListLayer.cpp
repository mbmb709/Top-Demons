#include "GDLListLayer.hpp"
#include "../../API/Levels/Levels.hpp"
#include "../../Cache/Levels/Levels.hpp"
#include "../../Cache/GameLevels/GameLevels.hpp"
#include "../../Cache/Users/Users.hpp"
#include "../../Filters/Filters.hpp"
#include "../../Popups/FilterPopup/FilterPopup.hpp"
#include "../../Events/PopulateListEvent.hpp"
#include "../../Events/DemonlistLoadedEvent.hpp"
#include "../../Models/APIError.hpp"

#include <algorithm>
#include <cctype>
#include <random>
#include <unordered_map>

GDLListLayer* GDLListLayer::create() {
	auto ret = new GDLListLayer();
	if (ret->init()) {
		ret->autorelease();
		return ret;
	}
	CC_SAFE_DELETE(ret);
	return nullptr;
}

CCScene* GDLListLayer::scene() {
	auto ret = CCScene::create();
	AppDelegate::get()->m_runningScene = ret;
	ret->addChild(GDLListLayer::create());
	return ret;
}

constexpr const char* TOP_DEMONS_INFO =
"Browse a verified <cg>Top 150</c> of rated Extreme Demons. Invalid IDs are "
"replaced with a rated Extreme Demon that has the same name. Use search and "
"filters to find a level, or press refresh to verify the ranking again.";
bool GDLListLayer::init() {
	if (!CCLayer::init()) return false;

	setID("GDLListLayer");

	auto winSize = CCDirector::get()->getWinSize();

	auto gdlBG = CCSprite::create("global-list-bg.png"_spr);
	gdlBG->setAnchorPoint({ 0.5f, 0.5f });
	gdlBG->setScale(Utils::calculateCoverScale(winSize, gdlBG->getContentSize()));
	gdlBG->setPosition({ winSize.width / 2, winSize.height / 2 });
	gdlBG->setZOrder(0);
	gdlBG->setID("gdl-backgrownd");
	addChild(gdlBG);

	m_levelsCountLabel = CCLabelBMFont::create("", "goldFont.fnt");
	m_levelsCountLabel->setAnchorPoint({ 1.0f, 1.0f });
	m_levelsCountLabel->setScale(0.45f);
	m_levelsCountLabel->setPosition({ winSize.width - 7.0f, winSize.height - 3.0f });
	m_levelsCountLabel->setID("level-count-label");
	addChild(m_levelsCountLabel);

	m_levelList = GJListLayer::create(
		nullptr,
		"Top Demons - Top 150",
		{ 0, 0, 0, 180 },
		356.0f, 220.0f, 0);
	m_levelList->setPosition(winSize / 2.0f - m_levelList->getContentSize() / 2.0f);
	m_levelList->setColor({ 194, 114, 62 });
	m_levelList->setID("GJListLayer");
	addChild(m_levelList, 5);

	m_errorMessage = CCLabelBMFont::create("", "bigFont.fnt");
	m_errorMessage->setScale(0.6f);
	m_errorMessage->setPosition(m_levelList->getContentSize() / 2.0f);
	m_errorMessage->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);
	m_errorMessage->setID("error-message");
	m_levelList->addChild(m_errorMessage, 5);

	m_searchBarMenu = CCNode::create();
	m_searchBarMenu->setContentSize({ 356.0f, 30.0f });
	m_searchBarMenu->setPosition({ 0.0f, m_levelList->getContentHeight() - m_searchBarMenu->getContentHeight() });
	m_searchBarMenu->setID("search-bar-menu");
	m_levelList->addChild(m_searchBarMenu);

	auto searchBarBG = CCLayerColor::create({ 194, 114, 62, 255 }, 356.0f, 30.0f);
	searchBarBG->setID("search-bar-backgrownd");
	m_searchBarMenu->addChild(searchBarBG);

	auto searchBarContainer = CCMenu::create();
	searchBarContainer->setLayout(
		RowLayout::create()
		->setGap(5.0f)
		->setAutoScale(false)
		->setCrossAxisOverflow(false)
	);
	searchBarContainer->setContentSize(m_searchBarMenu->getContentSize());
	searchBarContainer->setAnchorPoint({ 0.0f, 0.0f });
	searchBarContainer->setPosition({ 0.0f, 0.0f });
	searchBarContainer->setID("search-bar-container");
	m_searchBarMenu->addChild(searchBarContainer);

	m_searchBar = TextInput::create(367.0f, "Search levels...");
	m_searchBar->setMaxCharCount(32);
	m_searchBar->setTextAlign(TextInputAlign::Left);
	m_searchBar->getInputNode()->setLabelPlaceholderScale(0.70f);
	m_searchBar->getInputNode()->setMaxLabelScale(0.70f);
	m_searchBar->setScale(0.75f);
	m_searchBar->setID("search-bar");
	searchBarContainer->addChild(m_searchBar);

	auto searchBtn = CCMenuItemExt::createSpriteExtraWithFrameName("gj_findBtn_001.png", 0.7f, [this](auto) { search(); });
	searchBtn->setID("search-button");
	searchBarContainer->addChild(searchBtn);

	auto filterBtn = CCMenuItemExt::createSpriteExtraWithFilename("GJ_button_01.png", 0.54f, [this](auto) {
		FilterPopup::create()->show();
	});
	auto filterIcon = CCSprite::createWithSpriteFrameName("GJ_filterIcon_001.png");
	filterIcon->setPosition(filterBtn->getContentSize() / 2.0f);
	filterIcon->setScale(0.54f);
	filterBtn->addChild(filterIcon);
	filterBtn->setID("filter-button");
	searchBarContainer->addChild(filterBtn);

	searchBarContainer->updateLayout();

	auto btnsMenu = CCMenu::create();
	btnsMenu->setPosition({ 0.0f, 0.0f });
	btnsMenu->setID("buttons-menu");
	addChild(btnsMenu, 2);

	auto backSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
	m_backBtn = CCMenuItemExt::createSpriteExtra(backSpr, [this](auto) { onBack(); });
	m_backBtn->setPosition({ 24.0f, winSize.height - 23.0f });
	m_backBtn->setID("back-button");
	btnsMenu->addChild(m_backBtn);

	auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
	m_leftBtn = CCMenuItemExt::createSpriteExtra(leftSpr, [this](auto) { page(m_page - 1); });
	m_leftBtn->setPosition({ 24.0f, winSize.height / 2.0f });
	m_leftBtn->setID("prev-page-button");
	btnsMenu->addChild(m_leftBtn);

	auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
	rightSpr->setFlipX(true);
	m_rightBtn = CCMenuItemExt::createSpriteExtra(rightSpr, [this](auto) { page(m_page + 1); });
	m_rightBtn->setPosition({ winSize.width - 24.0f, winSize.height / 2.0f });
	m_rightBtn->setID("next-page-button");
	btnsMenu->addChild(m_rightBtn);

	m_infoBtn = InfoAlertButton::create("Top Demons", TOP_DEMONS_INFO, 1.0f);
	m_infoBtn->setPosition({ 30.0f, 30.0f });
	m_infoBtn->setID("info-button");
	btnsMenu->addChild(m_infoBtn);

	auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
	auto refreshBtn = CCMenuItemExt::createSpriteExtra(refreshSpr, [this](auto) {
		m_page = 1;
		m_searchBar->setString("");
		GDL::Filters::clearFilters();

		showLoading();
		GDL::Cache::Levels::clear();
		GDL::API::Levels::getDemonlist();
	});
	refreshBtn->setPosition({ winSize.width - refreshSpr->getContentWidth() / 2.0f - 4.0f, refreshSpr->getContentHeight() / 2.0f + 4.0f });
	refreshBtn->setID("refresh-button");
	btnsMenu->addChild(refreshBtn);

	auto pageSpr = CCSprite::create("GJ_button_02.png");
	pageSpr->setScale(0.7f);
	m_pageLabel = CCLabelBMFont::create("1", "bigFont.fnt");
	m_pageLabel->setScale(0.8f);
	m_pageLabel->setPosition(pageSpr->getContentSize() / 2.0f);
	pageSpr->addChild(m_pageLabel);
	m_pageBtn = CCMenuItemExt::createSpriteExtra(pageSpr, [this](auto) {
		auto popup = SetIDPopup::create(m_page, 1, calculateMaxPage(), "Go to Page", "Go", true, 1, 60.0f, false, false);
		popup->m_delegate = this;
		popup->show();
	});
	m_pageBtn->setID("page-button");

	m_randomBtn = CCMenuItemExt::createSpriteExtraWithFilename("BI_randomBtn_001.png"_spr, 0.9f, [this](auto) {
		static std::mt19937 mt(std::random_device{}());
		page(std::uniform_int_distribution<int>(0, (m_searchResults.size() - 1) / m_lvlsPerPage)(mt));
	});
	m_randomBtn->setID("random-button");
	
	m_pageMenu = CCMenu::createWithItem(m_pageBtn);
	m_pageMenu->setLayout(
		ColumnLayout::create()
		->setAxisAlignment(AxisAlignment::End)
		->setAxisReverse(true)
	);
	m_pageMenu->setAnchorPoint({ 1.0f, 1.0f });
	m_pageMenu->setPosition({ winSize.width - 3.0f, winSize.height - 25.5f });
	m_pageMenu->addChild(m_randomBtn);
	m_pageMenu->setID("page-menu");
	btnsMenu->addChild(m_pageMenu);

	auto lastArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
	lastArrow->setFlipX(true);
	auto otherLastArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
	otherLastArrow->setPosition(lastArrow->getContentSize() / 2.0f + CCPoint{ 20.0f, 0.0f });
	otherLastArrow->setFlipX(true);
	lastArrow->addChild(otherLastArrow);
	lastArrow->setScale(0.4f);
	m_lastBtn = CCMenuItemExt::createSpriteExtra(lastArrow, [this](auto) { page(calculateMaxPage()); });
	m_lastBtn->setID("last-button");
	m_pageMenu->addChild(m_lastBtn);

	m_pageMenu->updateLayout();

	auto firstArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
	auto otherFirstArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
	otherFirstArrow->setPosition(firstArrow->getContentSize() / 2.0f - ccp( 20.0f, 0.0f ));
	firstArrow->addChild(otherFirstArrow);
	firstArrow->setScale(0.4f);
	m_firstBtn = CCMenuItemExt::createSpriteExtra(firstArrow, [this](auto) { page(1); });
	m_firstBtn->setID("first-button");
	m_firstBtn->setPosition({ 17.5f, m_pageMenu->getPositionY() - m_pageMenu->getContentHeight() + m_lastBtn->getPositionY() });
	btnsMenu->addChild(m_firstBtn);

	m_loadingSpinner = LoadingSpinner::create(65.0f);
	m_loadingSpinner->setAnchorPoint({ 0.5f, 0.5f });
	m_loadingSpinner->setPosition(winSize / 2.0f);
	m_loadingSpinner->setID("lodaing-spinner");
	addChild(m_loadingSpinner, 10);

	setKeypadEnabled(true);
	setKeyboardEnabled(true);
	showLoading();

	m_demonlistLoadListener = DemonlistLoadedEvent().listen(
		[this](Result<const std::vector<int>&, APIError> result) {
			if (result.isOk()) {
				m_gdlLevels = result.unwrap();
				if (m_gdlLevels.size() > 150) {
					m_gdlLevels.resize(150);
				}
				startLevelValidation();
			}
			else {
				m_loadingSpinner->setVisible(false);

				auto error = result.err().value();
				auto errorStr = fmt::format("Failed to load Top Demons.\nError: {}", error.typeAsString());
				if (error.message != APIMessage::None) {
					errorStr += fmt::format(", Message: {}", error.messageAsString());
				}
				
				m_errorMessage->setString(errorStr.c_str());
			}
		}
	);

	m_populateListListener = PopulateListEvent().listen(
		[this]() {
			populateList();
		}
	);

	Settings::setShouldLoadPlacement(true);
	GDL::API::Levels::getDemonlist();

	return true;
}

std::string GDLListLayer::normalizeLevelName(std::string_view name) {
	std::string normalized;
	normalized.reserve(name.size());

	bool pendingSpace = false;
	for (unsigned char character : name) {
		if (std::isspace(character)) {
			pendingSpace = !normalized.empty();
			continue;
		}

		if (pendingSpace) {
			normalized.push_back(' ');
			pendingSpace = false;
		}

		normalized.push_back(
			static_cast<char>(std::tolower(character))
		);
	}

	return normalized;
}

bool GDLListLayer::isRatedExtremeDemon(GJGameLevel* level) const {
	return
		level &&
		level->m_stars.value() == 10 &&
		level->m_demon.value() != 0 &&
		level->m_demonDifficulty ==
			static_cast<int>(DemonDifficultyType::ExtremeDemon);
}

void GDLListLayer::cacheGameLevel(GJGameLevel* level) {
	if (!level) return;

	GDL::Cache::GameLevels::setGameLevel(GameLevel{
		level->m_levelName.c_str(),
		level->m_creatorName.c_str(),
		level->m_levelID.value(),
		level->m_stars.value() > 0
	});
}

void GDLListLayer::startLevelValidation() {
	showLoading();

	m_levelsToValidate = m_gdlLevels;
	m_gdlLevels.clear();
	m_resolvedLevelIDs.assign(m_levelsToValidate.size(), 0);
	m_invalidLevels.clear();
	m_acceptedLevelIDs.clear();
	m_validationBatch = 0;
	m_invalidLevelIndex = 0;
	m_validationRetryCount = 0;
	m_replacementSearchPage = 0;
	m_validationStage = ValidationStage::LevelIDs;

	GDL::Cache::GameLevels::clear();

	if (m_levelsToValidate.empty()) {
		finishLevelValidation();
		return;
	}

	updateValidationStatus();
	loadValidationBatch(0.f);
}

void GDLListLayer::updateValidationStatus() {
	if (m_validationStage == ValidationStage::LevelIDs) {
		auto checked = std::min(
			m_validationBatch * VALIDATION_BATCH_SIZE,
			m_levelsToValidate.size()
		);
		m_errorMessage->setString(
			fmt::format(
				"Checking Extreme Demons...\n{} / {}",
				checked,
				m_levelsToValidate.size()
			).c_str()
		);
	}
	else if (
		m_validationStage == ValidationStage::ReplacementSearch
	) {
		m_errorMessage->setString(
			fmt::format(
				"Finding correct rated versions...\n{} / {}",
				std::min(
					m_invalidLevelIndex + 1,
					m_invalidLevels.size()
				),
				m_invalidLevels.size()
			).c_str()
		);
	}
}

void GDLListLayer::loadValidationBatch(float) {
	if (m_validationStage != ValidationStage::LevelIDs) return;

	size_t begin = m_validationBatch * VALIDATION_BATCH_SIZE;
	if (begin >= m_levelsToValidate.size()) {
		m_validationStage = ValidationStage::ReplacementSearch;
		m_invalidLevelIndex = 0;
		m_validationRetryCount = 0;
		m_replacementSearchPage = 0;

		if (m_invalidLevels.empty()) {
			finishLevelValidation();
		}
		else {
			updateValidationStatus();
			searchNextReplacement(0.f);
		}
		return;
	}

	size_t end = std::min(
		begin + VALIDATION_BATCH_SIZE,
		m_levelsToValidate.size()
	);

	std::string searchQuery;
	for (size_t i = begin; i < end; i++) {
		if (i > begin) searchQuery += ",";
		searchQuery += std::to_string(m_levelsToValidate[i]);
	}

	auto searchObject = GJSearchObject::create(
		SearchType::Type26,
		searchQuery
	);
	auto gameLevelManager = GameLevelManager::get();
	gameLevelManager->m_levelManagerDelegate = this;
	gameLevelManager->getOnlineLevels(searchObject);
}

void GDLListLayer::processValidationBatch(CCArray* levels) {
	if (m_validationStage != ValidationStage::LevelIDs) return;

	std::unordered_map<int, GJGameLevel*> returnedLevels;
	for (size_t i = 0; i < levels->count(); i++) {
		auto level = static_cast<GJGameLevel*>(
			levels->objectAtIndex(i)
		);
		if (!level) continue;

		returnedLevels.insert_or_assign(
			level->m_levelID.value(),
			level
		);
	}

	size_t begin = m_validationBatch * VALIDATION_BATCH_SIZE;
	size_t end = std::min(
		begin + VALIDATION_BATCH_SIZE,
		m_levelsToValidate.size()
	);

	for (size_t index = begin; index < end; index++) {
		int levelID = m_levelsToValidate[index];
		auto metadata = GDL::Cache::Levels::getLevel(levelID);
		if (!metadata) {
			GDL::Cache::Levels::removeLevel(levelID);
			continue;
		}

		auto returned = returnedLevels.find(levelID);
		auto level =
			returned == returnedLevels.end() ? nullptr : returned->second;

		bool correctName =
			level &&
			normalizeLevelName(level->m_levelName.c_str()) ==
				normalizeLevelName(metadata->name);

		if (
			correctName &&
			isRatedExtremeDemon(level) &&
			m_acceptedLevelIDs.insert(levelID).second
		) {
			m_resolvedLevelIDs[index] = levelID;
			cacheGameLevel(level);
		}
		else {
			m_invalidLevels.push_back({
				index,
				levelID,
				metadata->name
			});
		}
	}

	m_validationBatch++;
	m_validationRetryCount = 0;
	updateValidationStatus();
	scheduleOnce(
		static_cast<cocos2d::SEL_SCHEDULE>(
			&GDLListLayer::loadValidationBatch
		),
		0.35f
	);
}

void GDLListLayer::searchNextReplacement(float) {
	if (
		m_validationStage != ValidationStage::ReplacementSearch
	) {
		return;
	}

	if (m_invalidLevelIndex >= m_invalidLevels.size()) {
		finishLevelValidation();
		return;
	}

	updateValidationStatus();

	auto& invalidLevel = m_invalidLevels[m_invalidLevelIndex];
	auto searchObject = GJSearchObject::create(
		SearchType::Search,
		invalidLevel.name
	);
	if (m_replacementSearchPage > 0) {
		if (auto pageObject = searchObject->getPageObject(
			m_replacementSearchPage
		)) {
			searchObject = pageObject;
		}
	}
	auto gameLevelManager = GameLevelManager::get();
	gameLevelManager->m_levelManagerDelegate = this;
	gameLevelManager->getOnlineLevels(searchObject);
}

void GDLListLayer::processReplacementSearch(CCArray* levels) {
	if (
		m_validationStage != ValidationStage::ReplacementSearch ||
		m_invalidLevelIndex >= m_invalidLevels.size()
	) {
		return;
	}

	auto& invalidLevel = m_invalidLevels[m_invalidLevelIndex];
	auto expectedName = normalizeLevelName(invalidLevel.name);
	GJGameLevel* bestMatch = nullptr;

	for (size_t i = 0; i < levels->count(); i++) {
		auto candidate = static_cast<GJGameLevel*>(
			levels->objectAtIndex(i)
		);
		if (!candidate) continue;

		int candidateID = candidate->m_levelID.value();
		if (
			m_acceptedLevelIDs.contains(candidateID) ||
			!isRatedExtremeDemon(candidate) ||
			normalizeLevelName(candidate->m_levelName.c_str()) !=
				expectedName
		) {
			continue;
		}

		if (
			!bestMatch ||
			candidate->m_downloads > bestMatch->m_downloads ||
			(
				candidate->m_downloads == bestMatch->m_downloads &&
				candidate->m_likes > bestMatch->m_likes
			)
		) {
			bestMatch = candidate;
		}
	}

	bool replacementApplied = false;
	if (bestMatch) {
		int replacementID = bestMatch->m_levelID.value();
		if (
			GDL::Cache::Levels::replaceLevelID(
				invalidLevel.oldLevelID,
				replacementID
			) &&
			m_acceptedLevelIDs.insert(replacementID).second
		) {
			m_resolvedLevelIDs[invalidLevel.index] = replacementID;
			cacheGameLevel(bestMatch);
			replacementApplied = true;
		}
	}

	if (
		!replacementApplied &&
		!bestMatch &&
		levels->count() >= 10 &&
		m_replacementSearchPage < 2
	) {
		m_replacementSearchPage++;
		m_validationRetryCount = 0;
		scheduleOnce(
			static_cast<cocos2d::SEL_SCHEDULE>(
				&GDLListLayer::searchNextReplacement
			),
			0.65f
		);
		return;
	}

	if (!replacementApplied) {
		skipCurrentInvalidLevel();
	}

	m_invalidLevelIndex++;
	m_validationRetryCount = 0;
	m_replacementSearchPage = 0;
	scheduleOnce(
		static_cast<cocos2d::SEL_SCHEDULE>(
			&GDLListLayer::searchNextReplacement
		),
		0.65f
	);
}

void GDLListLayer::skipCurrentInvalidLevel() {
	if (m_invalidLevelIndex >= m_invalidLevels.size()) return;

	auto& invalidLevel = m_invalidLevels[m_invalidLevelIndex];
	m_resolvedLevelIDs[invalidLevel.index] = 0;

	if (
		!m_acceptedLevelIDs.contains(invalidLevel.oldLevelID)
	) {
		GDL::Cache::Levels::removeLevel(invalidLevel.oldLevelID);
	}
}

void GDLListLayer::finishLevelValidation() {
	m_validationStage = ValidationStage::None;
	m_gdlLevels.clear();

	for (int levelID : m_resolvedLevelIDs) {
		if (levelID > 0) {
			m_gdlLevels.push_back(levelID);
		}
	}

	GDL::Cache::GameLevels::updateCachedAt();
	m_errorMessage->setScale(0.6f);
	m_errorMessage->setString("");

	m_levelsToValidate.clear();
	m_resolvedLevelIDs.clear();
	m_invalidLevels.clear();
	m_acceptedLevelIDs.clear();

	if (m_gdlLevels.empty()) {
		m_loadingSpinner->setVisible(false);
		m_errorMessage->setString(
			"No rated Extreme Demons could be verified."
		);
		return;
	}

	populateList();
}

void GDLListLayer::populateList() {
	GDL::Filters::applyFilters();
	m_searchResults.clear();

	if (m_query.empty() && GDL::Filters::getLevelFilters().isDefault()) {
		m_searchResults = m_gdlLevels;
	}
	else {
		m_searchResults = getSuitableLevels();
	}

	if (m_searchResults.empty()) {
		loadLevelsFinished(CCArray::create(), "", 0);
	}
	else {
		auto glm = GameLevelManager::get();
		glm->m_levelManagerDelegate = this;

		size_t begin = m_lvlsPerPage * (m_page-1);
		size_t end = std::min(
			static_cast<size_t>(m_lvlsPerPage * m_page),
			m_searchResults.size()
		);
		
		std::string searchQuery;
		for (size_t i = begin; i < end; i++) {
			if (i > begin) searchQuery += ",";
			searchQuery += std::to_string(m_searchResults[i]);
		}

		auto searchObject = GJSearchObject::create(SearchType::Type26);
		searchObject->m_searchQuery = searchQuery;

		std::string_view key = searchObject->getKey();
		if (auto storedLevels = glm->getStoredOnlineLevels(
			key.substr(std::max<ptrdiff_t>(0, key.size() - 256)).data()
		)) {
			loadLevelsFinished(storedLevels, "", 0);
			setupPageInfo("", "");
		}
		else {
			glm->getOnlineLevels(searchObject);
		}
	}
}

std::vector<int> GDLListLayer::getSuitableLevels() {
	auto& levelFilters = GDL::Filters::getLevelFilters();
	auto user = GDL::Cache::Users::getUser(levelFilters.userID);

	std::vector<int> suitableLevels;
	suitableLevels.reserve(m_gdlLevels.size());

	for (const auto& levelID : m_gdlLevels) {
		auto level = GDL::Cache::Levels::getLevel(levelID);
		if (!level) continue;

		LengthFilter levelLength = LengthFilter::None;
        if (level->length < 30) levelLength = LengthFilter::Short;
        else if (level->length < 60) levelLength = LengthFilter::Medium;
        else if (level->length < 120) levelLength = LengthFilter::Long;
        else if (level->length >= 120) levelLength = LengthFilter::XL;
        if (
            levelFilters.lengthFilter == LengthFilter::Custom &&
            level->length >= levelFilters.customLengthFilter[0] &&
            level->length <= levelFilters.customLengthFilter[1]
        ) levelLength = LengthFilter::Custom;

		bool levelIsCompleted = false;
        if (user) {
            auto& completedList = user->getCompletedList();

            levelIsCompleted = std::ranges::find_if(
                completedList,
                [internalID = level->id](const GDLBasicLevel& record) {
                    return record.id == internalID;
                }
            ) != completedList.end();
        }

		auto gameLevel = GDL::Cache::GameLevels::getGameLevel(level->ingameID);
        
        bool byDifficulty =
            levelFilters.diffFilter == DifficultyFilter::None ||
            (
                levelFilters.diffFilter == DifficultyFilter::Top75 &&
                level->placement <= 75
            ) ||
            (
                levelFilters.diffFilter == DifficultyFilter::Top150 &&
                level->placement <= 150
            ) ||
            (
                levelFilters.diffFilter == DifficultyFilter::Custom &&
                level->placement >= levelFilters.customDiffFilter[0] &&
                level->placement <= levelFilters.customDiffFilter[1]
            );
        bool byLength = levelFilters.lengthFilter == levelLength || levelFilters.lengthFilter == LengthFilter::None;

		bool byRate =
        levelFilters.rated || levelFilters.unrated ?
            gameLevel ?
                levelFilters.rated ? gameLevel->rated : !gameLevel->rated
            : false
        : true;

        bool byPlayer = levelFilters.completedBy ? levelIsCompleted : true;

        bool byCreator =
        levelFilters.createdBy ?
            gameLevel ?
                gameLevel->creatorName == levelFilters.holder
            : false
        : true;

		if (
			byDifficulty && byLength && byRate && byPlayer && byCreator &&
			level->contains(m_query)
		) {
			suitableLevels.push_back(levelID);
		}
	}

	return suitableLevels;
}

void GDLListLayer::search() {
	auto query = m_searchBar->getString();
	if (m_query != query) {
		showLoading();

		m_page = 1;
		m_query = query;
		populateList();
	}
}

void GDLListLayer::loadLevelsFinished(CCArray* levels, const char*, int) {
	if (!levels) {
		loadLevelsFailed("");
		return;
	}

	if (m_validationStage == ValidationStage::LevelIDs) {
		processValidationBatch(levels);
		return;
	}

	if (
		m_validationStage == ValidationStage::ReplacementSearch
	) {
		processReplacementSearch(levels);
		return;
	}

	if (auto listView = m_levelList->m_listView) {
		listView->removeFromParent();
		listView->release();
	}

	auto listView = CustomListView::create(levels, BoomListType::Level, 190.0f, 356.0f);
	listView->retain();
	m_levelList->addChild(listView, 6, 9);
	m_levelList->m_listView = listView;

	m_searchBarMenu->setVisible(true);
	m_levelsCountLabel->setVisible(true);
	m_loadingSpinner->setVisible(false);

	if (m_searchResults.size() > m_lvlsPerPage) {
		int maxPage = calculateMaxPage();
		
		m_leftBtn->setVisible(m_page > 1);
		m_rightBtn->setVisible(m_page < maxPage);
		m_firstBtn->setVisible(m_page > 1);
		m_lastBtn->setVisible(m_page < maxPage);
		m_pageMenu->setVisible(true);
	}
	m_pageLabel->limitLabelWidth( 32.0f, 0.8f, 0.4f );
}

void GDLListLayer::loadLevelsFailed(char const* key) {
	if (m_validationStage == ValidationStage::LevelIDs) {
		if (m_validationRetryCount < 2) {
			m_validationRetryCount++;
			scheduleOnce(
				static_cast<cocos2d::SEL_SCHEDULE>(
					&GDLListLayer::loadValidationBatch
				),
				1.25f
			);
			return;
		}

		m_validationStage = ValidationStage::None;
		m_loadingSpinner->setVisible(false);
		m_errorMessage->setString(
			"Could not verify the Extreme Demon levels.\nTry Refresh."
		);

		auto gameLevelManager = GameLevelManager::get();
		if (gameLevelManager->m_levelManagerDelegate == this) {
			gameLevelManager->m_levelManagerDelegate = nullptr;
		}
		return;
	}

	if (
		m_validationStage == ValidationStage::ReplacementSearch
	) {
		if (m_validationRetryCount < 2) {
			m_validationRetryCount++;
			scheduleOnce(
				static_cast<cocos2d::SEL_SCHEDULE>(
					&GDLListLayer::searchNextReplacement
				),
				1.25f
			);
			return;
		}

		skipCurrentInvalidLevel();
		m_invalidLevelIndex++;
		m_validationRetryCount = 0;
		m_replacementSearchPage = 0;
		scheduleOnce(
			static_cast<cocos2d::SEL_SCHEDULE>(
				&GDLListLayer::searchNextReplacement
			),
			0.65f
		);
		return;
	}

	FLAlertLayer::create("Loading failed", "Something wrong.", "Ok")->show();
}

void GDLListLayer::loadLevelsFailed(char const* key, int) {
	loadLevelsFailed(key);
}

void GDLListLayer::page(int page) {
	int maxPage = calculateMaxPage();
	m_page = page <= maxPage ? (page < 1 ? 1 : page) : maxPage;

	showLoading();
	populateList();
}

void GDLListLayer::setupPageInfo(gd::string, const char*) {
	m_levelsCountLabel->setString(
		fmt::format("{} to {} of {}",
			m_lvlsPerPage * (m_page-1) + 1,
			std::min(static_cast<int>(m_searchResults.size()), m_page * m_lvlsPerPage),
			m_searchResults.size()).c_str()
		);
	m_levelsCountLabel->limitLabelWidth( 100.0f, 0.6f, 0.0f );
}

void GDLListLayer::setIDPopupClosed(SetIDPopup*, int page) {
	m_page = std::clamp(page, 1, calculateMaxPage());
	showLoading();
	populateList();
}

void GDLListLayer::showLoading() {
	m_pageLabel->setString(fmt::to_string(m_page).c_str());
	m_loadingSpinner->setVisible(true);

	if (auto listView = m_levelList->m_listView) listView->setVisible(false);
	m_searchBarMenu->setVisible(false);
	m_levelsCountLabel->setVisible(false);
	m_leftBtn->setVisible(false);
	m_rightBtn->setVisible(false);
	m_firstBtn->setVisible(false);
	m_lastBtn->setVisible(false);
	m_pageMenu->setVisible(false);
}

void GDLListLayer::keyDown(enumKeyCodes key, double d) {
	switch (key) {
		case KEY_Left:
	case CONTROLLER_Left:
		if (m_leftBtn->isVisible()) page(m_page - 1);
		break;
	case KEY_Right:
	case CONTROLLER_Right:
		if (m_rightBtn->isVisible()) page(m_page + 1);
		break;
	case KEY_Enter:
		search();
		break;
	default:
		CCLayer::keyDown(key, d);
		break;
	}
}

void GDLListLayer::onBack() {
	Settings::setShouldLoadPlacement(false);
	CCDirector::get()->popSceneWithTransition(0.5f, kPopTransitionFade);
}

void GDLListLayer::keyBackClicked() {
	onBack();
}

void GDLListLayer::onExit() {
	auto glm = GameLevelManager::get();
	if (glm->m_levelManagerDelegate == this) {
		glm->m_levelManagerDelegate = nullptr;
	}

	if (m_searchBar) m_searchBar->defocus();

	CCLayer::onExit();
}

int GDLListLayer::calculateMaxPage() {
	return std::max(1, (static_cast<int>(m_searchResults.size()) + m_lvlsPerPage - 1) / m_lvlsPerPage);
}
