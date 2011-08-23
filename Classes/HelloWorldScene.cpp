//
//  HelloWorldScene.cpp
//  TileBasedGame
//
//  Created by Clawoo on 8/17/11.
//  Copyright __MyCompanyName__ 2011. All rights reserved.
//

#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"
#include <string>

using namespace std;
using namespace CocosDenshion;
USING_NS_CC;

CCScene* HelloWorld::scene()
{
	// 'scene' is an autorelease object
	CCScene *scene = CCScene::node();
	
	// 'layer' is an autorelease object
	HelloWorld *layer = HelloWorld::node();

	// add layer as a child to scene
	scene->addChild(layer);

	HelloWorldHud *hud = HelloWorldHud::node();
	hud->retain();
	scene->addChild(hud);
	layer->setHud(hud);
	
	// return the scene
	return scene;
}

HelloWorld::~HelloWorld()
{
	CC_SAFE_RELEASE_NULL(_tileMap);
	CC_SAFE_RELEASE_NULL(_background);
	CC_SAFE_RELEASE_NULL(_foreground);
	CC_SAFE_RELEASE_NULL(_player);
	CC_SAFE_RELEASE_NULL(_meta);
	CC_SAFE_RELEASE_NULL(_hud);
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
	if ( !CCLayer::init() )
	{
		return false;
	}
	
	SimpleAudioEngine::sharedEngine()->preloadEffect("pickup.caf");
	SimpleAudioEngine::sharedEngine()->preloadEffect("hit.caf");
	SimpleAudioEngine::sharedEngine()->preloadEffect("move.caf");
	SimpleAudioEngine::sharedEngine()->playBackgroundMusic("TileMap.caf");
	
	_tileMap = CCTMXTiledMap::tiledMapWithTMXFile("TileMap.tmx");
    _tileMap->retain();
    
	_background = _tileMap->layerNamed("Background");
    _background->retain();
    
	_foreground = _tileMap->layerNamed("Foreground");
    _foreground->retain();
	
	_meta = _tileMap->layerNamed("Meta");
	_meta->retain();
	
	CCTMXObjectGroup *objects = _tileMap->objectGroupNamed("Objects");
	CCAssert(objects != NULL, "'Objects' object group not found");
	
	CCStringToStringDictionary *spawnPoint = objects->objectNamed("SpawnPoint");
	CCAssert(spawnPoint != NULL, "SpawnPoint object not found");
	int x = spawnPoint->objectForKey("x")->toInt();
	int y = spawnPoint->objectForKey("y")->toInt();
	
    this->addChild(_tileMap);
	
	CCMutableArray<CCStringToStringDictionary*> *allObjects = objects->getObjects();
	CCMutableArray<CCStringToStringDictionary*>::CCMutableArrayIterator it;
	for (it = allObjects->begin(); it != allObjects->end(); ++it)
	{
		if ((*it)->objectForKey(std::string("Enemy")) != NULL)
		{
			int x = (*it)->objectForKey("x")->toInt();
			int y = (*it)->objectForKey("y")->toInt();
			this->addEnemyAt(x, y);
		}
	}

	_player = CCSprite::spriteWithFile("Player.png");
	_player->retain();
	_player->setPosition(ccp (x, y));
	this->addChild(_player);
	
	this->setViewpointCenter(_player->getPosition());
	
	this->setIsTouchEnabled(true);
	
	_numCollected = 0;
	
	return true;
}

void HelloWorld::setViewpointCenter(CCPoint position)
{
	CCSize winSize = CCDirector::sharedDirector()->getWinSize();
	
	int x = MAX(position.x, winSize.width / 2);
	int y = MAX(position.y, winSize.height / 2);
	
	x = MIN(x, (_tileMap->getMapSize().width * _tileMap->getTileSize().width) - winSize.width/2);
	x = MIN(x, (_tileMap->getMapSize().height * _tileMap->getTileSize().height) - winSize.height/2);
	
	CCPoint actualPosition = ccp(x, y);
	
	CCPoint centerOfView = ccp(winSize.width/2, winSize.height/2);
	
	CCPoint viewPoint = ccpSub(centerOfView, actualPosition);
	
	this->setPosition(viewPoint);
}

void HelloWorld::registerWithTouchDispatcher()
{
	CCTouchDispatcher::sharedDispatcher()->addTargetedDelegate(this, 0, true);
}

bool HelloWorld::ccTouchBegan(CCTouch *touch, CCEvent *event)
{
	return true;
}

void HelloWorld::ccTouchEnded(CCTouch *touch, CCEvent *event)
{
	CCPoint touchLocation = touch->locationInView(touch->view());
	touchLocation = CCDirector::sharedDirector()->convertToGL(touchLocation);
	touchLocation = this->convertToNodeSpace(touchLocation);
	
	CCPoint playerPos = _player->getPosition();
	CCPoint diff = ccpSub(touchLocation, playerPos);
	
	// using fabs because using abs throws a "abs in ambiguous" error
	if (fabs(diff.x) > fabs(diff.y)) {
        if (diff.x > 0) {
            playerPos.x += _tileMap->getTileSize().width;
        } else {
            playerPos.x -= _tileMap->getTileSize().width; 
        }    
    } else {
        if (diff.y > 0) {
            playerPos.y += _tileMap->getTileSize().height;
        } else {
            playerPos.y -= _tileMap->getTileSize().height;
        }
    }
	
	if (playerPos.x <= (_tileMap->getMapSize().width * _tileMap->getTileSize().width) &&
        playerPos.y <= (_tileMap->getMapSize().height * _tileMap->getTileSize().height) &&
        playerPos.y >= 0 &&
        playerPos.x >= 0 ) 
    {
		this->setPlayerPosition(playerPos);
    }
	
	this->setViewpointCenter(_player->getPosition());
}

void HelloWorld::setPlayerPosition(cocos2d::CCPoint position)
{
	CCPoint tileCoord = this->tileCoordForPosition(position);
	int tileGid = _meta->tileGIDAt(tileCoord);
	if (tileGid)
	{
		CCDictionary<std::string, CCString*> *properties = _tileMap->propertiesForGID(tileGid);
		if (properties)
		{
			CCString *collision = properties->objectForKey("Collidable");
			if (collision && (collision->toStdString().compare("True") == 0))
			{
				SimpleAudioEngine::sharedEngine()->playEffect("hit.caf");
				return;
			}
			
			CCString *collectable = properties->objectForKey("Collectable");
			if (collectable && (collectable->toStdString().compare("True") == 0))
			{
				_meta->removeTileAt(tileCoord);
				_foreground->removeTileAt(tileCoord);
				
				_numCollected++;
				_hud->numCollectedChanged(_numCollected);
				SimpleAudioEngine::sharedEngine()->playEffect("pickup.caf");
			}
		}
	}
	SimpleAudioEngine::sharedEngine()->playEffect("move.caf");
	_player->setPosition(position);
}

CCPoint HelloWorld::tileCoordForPosition(cocos2d::CCPoint position)
{
	int x = position.x / _tileMap->getTileSize().width;
    int y = ((_tileMap->getMapSize().height * _tileMap->getTileSize().height) - position.y) / _tileMap->getTileSize().height;
    return ccp(x, y);
}

void HelloWorld::addEnemyAt(int x, int y)
{
	CCSprite *enemy = CCSprite::spriteWithFile("enemy1.png");
	enemy->setPosition(ccp(x, y));
	this->addChild(enemy);
}