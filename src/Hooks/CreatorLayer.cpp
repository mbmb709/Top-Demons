#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include "../Layers/GDLListLayer/GDLListLayer.hpp"

using namespace geode::prelude;

class $modify(TopDemonsCreatorLayer, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;

        auto menu = getChildByID("creator-buttons-menu");
        if (!menu) {
            log::warn("Top Demons: creator-buttons-menu was not found");
            return true;
        }

        auto background = ButtonSprite::create(
            "Top 150",
            124,
            true,
            "bigFont.fnt",
            "GJ_button_01.png",
            34.f,
            0.58f
        );

        if (background->m_label) {
            background->m_label->setPositionX(
                background->m_label->getPositionX() - 8.f
            );
        }

        if (auto face = CCSprite::createWithSpriteFrameName(
            "diffIcon_10_btn_001.png"
        )) {
            face->setScale(0.34f);
            face->setPosition({
                background->getContentWidth() - 15.f,
                background->getContentHeight() / 2.f
            });
            face->setID("extreme-demon-icon"_spr);
            background->addChild(face, 2);
        }

        auto button = CCMenuItemSpriteExtra::create(
            background,
            this,
            menu_selector(TopDemonsCreatorLayer::onTop150)
        );
        button->setID("top-150-demons-button"_spr);
        menu->addChild(button);
        menu->updateLayout();

        return true;
    }

    void onTop150(CCObject*) {
        CCDirector::get()->pushScene(
            CCTransitionFade::create(0.35f, GDLListLayer::scene())
        );
    }
};
