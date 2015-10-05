//
//  GB2ShapeCache-x.cpp
//  
//  Loads physics sprites created with http://www.PhysicsEditor.de
//  To be used with cocos2d-x
//
//  Generic Shape Cache for box2d
//
//  Created by Thomas Broquist
//
//      http://www.PhysicsEditor.de
//      http://texturepacker.com
//      http://www.code-and-web.de
//  
//  All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//  
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//  
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//

#include "GB2ShapeCache-x.h"
#include "Box2D/Box2d.h"

USING_NS_CC;

/**
 * Internal class to hold the fixtures
 */
class FixtureDef {
public:
    FixtureDef()
    : next(NULL) {}
    
    ~FixtureDef() {
        delete next;
        delete fixture.shape;
    }
    
    FixtureDef *next;
    b2FixtureDef fixture;
    int callbackData;
};

class BodyDef
{
public:
	BodyDef()
	: fixtures(NULL) {}
	
	~BodyDef() {
		if (fixtures)
			delete fixtures;
	}
	
	FixtureDef *fixtures;
	Point anchorPoint;
};

static GB2ShapeCache *_sharedGB2ShapeCache = NULL;

GB2ShapeCache* GB2ShapeCache::getInstance()
{
	if (!_sharedGB2ShapeCache) {
		_sharedGB2ShapeCache = new GB2ShapeCache();
        _sharedGB2ShapeCache->init();
	}
	
	return _sharedGB2ShapeCache;
}

bool GB2ShapeCache::init()
{
	return true;
}

void GB2ShapeCache::reset()
{
	std::map<std::string, BodyDef *>::iterator iter;
	for (iter = shapeObjects.begin() ; iter != shapeObjects.end() ; ++iter)
    {
		delete iter->second;
	}
	shapeObjects.clear();
}

void GB2ShapeCache::addFixturesToBody(b2Body *body, const std::string &shape, float scale)
{
	std::map<std::string, BodyDef *>::iterator pos = shapeObjects.find(shape);
	assert(pos != shapeObjects.end());
	
	BodyDef *so = (*pos).second;

	FixtureDef *fix = so->fixtures;
    if(scale == 1.0f){
        while (fix) {
            body->CreateFixture(&fix->fixture);
            fix = fix->next;
        }
    }else{
        b2Vec2 vertices[b2_maxPolygonVertices];
        while(fix) {
            // make local copy of the fixture def
            b2FixtureDef fix2 = fix->fixture;
            
            // get the shape
            const b2Shape *s = fix2.shape;
            
            // clone & scale polygon
            const b2PolygonShape *p = dynamic_cast<const b2PolygonShape*>(s);
            if(p)
            {
                b2PolygonShape p2;
                for(int i=0; i<p->m_count; i++)
                {
                    vertices[i].x = p->m_vertices[i].x * scale;
                    vertices[i].y = p->m_vertices[i].y * scale;
                }
                p2.Set(vertices, p->m_count);
                fix2.shape = &p2;
            }
            
            // clone & scale circle
            const b2CircleShape *c = dynamic_cast<const b2CircleShape *>(s);
            if(c) {
                b2CircleShape c2;
                c2.m_radius = c->m_radius * scale;
                c2.m_p.x = c->m_p.x * scale;
                c2.m_p.y = c->m_p.y * scale;
                fix2.shape = &c2;
            }
            
            // add to body
            body->CreateFixture(&fix2);
            fix = fix->next;
        }
    }
}

cocos2d::Point GB2ShapeCache::anchorPointForShape(const std::string &shape)
{
	std::map<std::string, BodyDef *>::iterator pos = shapeObjects.find(shape);
	assert(pos != shapeObjects.end());
	
	BodyDef *bd = (*pos).second;
	return bd->anchorPoint;
}

void GB2ShapeCache::addShapesWithFile(const std::string &plist)
{
    const ValueMap &dict = FileUtils::getInstance()->getValueMapFromFile(plist.c_str());
    
    // not triggered - cocos2dx delivers empty dict if non was found
    CCASSERT(!dict.empty(), "Shape-file not found");
    CCASSERT(dict.size() != 0, "plist file empty or not existing");
	
	const ValueMap &metadataDict = dict.at("metadata").asValueMap();
    
    int format = metadataDict.at("format").asInt();
    CCASSERT(format == 1, "format not supported!");
    
    float ptmRatio = metadataDict.at("ptm_ratio").asFloat();
    CCLOG("ptmRatio = %f",ptmRatio);
    
	const ValueMap &bodyDict = dict.at("bodies").asValueMap();

    b2Vec2 vertices[b2_maxPolygonVertices];
    
    float contentScaleFactor = Director::getInstance()->getContentScaleFactor();
    
    //iterate body list
    for (auto ite = bodyDict.cbegin(); ite != bodyDict.cend(); ite ++) {
        const auto& bodyData = ite->second.asValueMap();
        const auto& bodyName = ite->first;
        
        BodyDef *bodyDef = new BodyDef();
        bodyDef->anchorPoint = PointFromString(bodyData.at("anchorpoint").asString());
        const ValueVector& fixtureList = bodyData.at("fixtures").asValueVector();
        FixtureDef **nextFixtureDef = &(bodyDef->fixtures);
        
        //iterate fixture list
        for (auto& fixtureItem : fixtureList) {
            b2FixtureDef basicData;
            const ValueMap& fixtureData = fixtureItem.asValueMap();
            
            basicData.filter.categoryBits = fixtureData.at("filter_categoryBits").asInt();
            
            basicData.filter.maskBits = fixtureData.at("filter_maskBits").asInt();
            basicData.filter.groupIndex = fixtureData.at("filter_groupIndex").asInt();
            basicData.friction = fixtureData.at("friction").asFloat();
            
            basicData.density = fixtureData.at("density").asFloat();
            
            basicData.restitution = fixtureData.at("restitution").asFloat();
            
            basicData.isSensor = fixtureData.at("isSensor").asBool();
            
            int callbackData = 0;
            auto iteCB = fixtureData.find("userdataCbValue");
            if(iteCB != fixtureData.end()){
				callbackData = iteCB->second.asInt();
            }

			std::string fixtureType = fixtureData.at("fixture_type").asString();

			if (fixtureType == "POLYGON") {
                const ValueVector& polygonsArray = fixtureData.at("polygons").asValueVector();
				
                for (auto& polygonEle : polygonsArray) {
                    const ValueVector &polygonArray = polygonEle.asValueVector();
                    
                    FixtureDef *fix = new FixtureDef();
                    fix->fixture = basicData; // copy basic data
                    fix->callbackData = callbackData;
                    
                    b2PolygonShape *polyshape = new b2PolygonShape();
                    int vindex = 0;
                    
                    assert(polygonArray.size() <= b2_maxPolygonVertices);
                    
                    for (auto& piter : polygonArray){
                        std::string verStr = piter.asString();
                        Point offset = PointFromString(verStr);
                        vertices[vindex].x = ((offset.x / ptmRatio)/contentScaleFactor) ;
                        vertices[vindex].y = ((offset.y / ptmRatio)/contentScaleFactor) ;
                        vindex++;
                    }
                    
                    polyshape->Set(vertices, vindex);
                    fix->fixture.shape = polyshape;
                    
                    // create a list
                    *nextFixtureDef = fix;
                    nextFixtureDef = &(fix->next);
                }
			}
            else if (fixtureType == "CIRCLE") {
				FixtureDef *fix = new FixtureDef();
                fix->fixture = basicData; // copy basic data
                fix->callbackData = callbackData;

                const ValueMap& circleData = fixtureData.at("circle").asValueMap();

                b2CircleShape *circleShape = new b2CircleShape();
				
                circleShape->m_radius = (circleData.at("radius").asFloat()/ ptmRatio)/contentScaleFactor;
				Point p = PointFromString(circleData.at("position").asString());
                circleShape->m_p = b2Vec2((p.x / ptmRatio)/contentScaleFactor,
                                          (p.y / ptmRatio)/contentScaleFactor);
                fix->fixture.shape = circleShape;
				
                // create a list
                *nextFixtureDef = fix;
                nextFixtureDef = &(fix->next);

			}
            else {
				CCAssert(0, "Unknown fixtureType");
			}
		}
        // add the body element to the hash
        shapeObjects[bodyName] = bodyDef;
    }
}