# GB2ShapeCache
GB2ShapeCache for cocos v3

Port from GB2ShapeCache-x original provide by http://www.code-and-web.de

* How to use it:
```

 $   PhysicsSprite *sprite = PhysicsSprite::create();
 $   
 $   b2BodyDef bodyDef;
 $   bodyDef.allowSleep = true;
 $   bodyDef.type = b2BodyType::b2_staticBody;
 $   
 $   b2Body *body = world->CreateBody(&bodyDef);
 $   
 $   GB2ShapeCache::getInstance()->addFixturesToBody(body, name);
 $   
 $   sprite->setB2Body(body);

```

* Contact me:
[1] Skype: kudo_108
[2] Facebook: http://facebook.com/kudo108
