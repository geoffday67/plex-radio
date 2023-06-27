#include "xml.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef ARDUINO
#include <Arduino.h>
#endif

void XmlParser::log(char const* plog) {
#ifdef ESP32
  //Serial.println(plog);
#else
  printf("--> %s\n", plog);
#endif
}

XmlParser::XmlParser() {
  pLexicalParser = new LexicalParser();
  ownLexical = true;
  init();
}

XmlParser::XmlParser(LexicalParser *pparser) {
  pLexicalParser = pparser;
  ownLexical = false;
  init();
}

XmlParser::~XmlParser() {
  if (ownLexical) {
    delete pLexicalParser;
  }
  delete pEntityParser;
}

void XmlParser::init() {
  pSubParser = 0;
  xmlCallback = 0;
  charCallback = 0;
  pEntityParser = new EntityParser;
  pdata = 0;
  reset();
}

void XmlParser::reset() {
  state = State::Idle;
  pbuffer = 0;
  charIndex = 0;
}

void XmlParser::setXmlCallback(XmlCallback callback) {
  this->xmlCallback = callback;
  if (pSubParser) {
    pSubParser->setXmlCallback(callback);
  }
}

void XmlParser::setCharCallback(CharCallback callback) {
  this->charCallback = callback;
  if (pSubParser) {
    pSubParser->setCharCallback(callback);
  }
}

void XmlParser::setData(void *pdata) {
  this->pdata = pdata;
  if (pSubParser) {
    pSubParser->setData(pdata);
  }
}

char *XmlParser::stripNamespace(char* pname) {
  return pname;
  /*char *pseparator = strchr(pname, ':');
  if (pseparator) {
    return pseparator + 1;
  } else {
    return pname;
  }*/
}

void XmlParser::startTagName() {
  pbuffer = name;
  charIndex = 0;
  state = State::TagName;
}

void XmlParser::endTagName() {
  pbuffer[charIndex] = 0;
  if (xmlCallback && !charCallback) {
    (*xmlCallback)(stripNamespace(name), 0, pdata);
  }
}

void XmlParser::startAttrName() {
  pbuffer = attrName;
  charIndex = 0;
  state = State::AttrName;
}

void XmlParser::endAttrName() {
  pbuffer[charIndex] = 0;
}

void XmlParser::startAttrValue() {
  pbuffer = attrValue;
  charIndex = 0;
  state = State::AttrValue;
}

void XmlParser::endAttrValue() {
  pbuffer[charIndex] = 0;
  if (xmlCallback && !charCallback) {
    (*xmlCallback)(stripNamespace(attrName), attrValue, pdata);
  }
}

void XmlParser::startBody() {
  pbuffer = value;
  charIndex = 0;
  pLexicalParser->valueOnly = true;
  state = State::Body;
}

void XmlParser::endBody() {
  pbuffer[charIndex] = 0;
  if (xmlCallback) {
    (*xmlCallback)(stripNamespace(name), value, pdata);
  }
}

void XmlParser::startSubParser() {
  pSubParser = new XmlParser(pLexicalParser);
  pSubParser->setXmlCallback(xmlCallback);
  pSubParser->setCharCallback(charCallback);
  pSubParser->setData(pdata);
}

void XmlParser::handleCharacter(char c) {
  char e = pEntityParser->mapChar(c);
  if (e) {
    if (charCallback) {
      (*charCallback)(name, e, pdata);
    } else if (pbuffer && charIndex < MAX_NAME - 1) {
      pbuffer[charIndex++] = e;
    }
  }
}

bool XmlParser::handleToken(int token) {
  switch(state) {
    case State::Idle:
      // Only < is relevant here.
      if (token == LexicalParser::START_OPEN_TAG) {
        startTagName();
      }
      break;
    case State::TagName:
      // Which tokens have special meaning inside a tag name?
      // > and whitespace. Both indicate end of tag name.
      // Whitespace => attribute follows.
      switch (token) {
        case LexicalParser::STOP_TAG:
          // We're about to start the body, the next character which will decide value or sub-tag.
          endTagName();
          startBody();
          break;
        case LexicalParser::WHITESPACE:
          // The tag name is ended and an attribute name is about to start.
          endTagName();
          startAttrName();
          break;
      }
      break;
    case State::AttrName:
      if (token == LexicalParser::ATTRIBUTE_SEPARATOR) {
        endAttrName();
        startAttrValue();
      }
      break;
    case State::AttrValue:
      switch (token) {
        case LexicalParser::STOP_TAG:
          endAttrValue();
          startBody();
          break;
        case LexicalParser::WHITESPACE:
          endAttrValue();
          startAttrName();
          break;
      }
      break;
    case State::Body:
      switch (token) {
        case LexicalParser::START_CLOSE_TAG:
          pLexicalParser->valueOnly = false;
          endBody();
          state = State::TagClose;
          break;
        case LexicalParser::START_OPEN_TAG:
          pLexicalParser->valueOnly = false;
          startSubParser();
          pSubParser->handleLexical(LexicalParser::START_OPEN_TAG);
          break;
      }
      break;
    case State::TagClose:
      switch (token) {
        case LexicalParser::STOP_TAG:
          state = State::Idle;
          return true;
      }
      break;
  }
  
  return false;
}

bool XmlParser::handleLexical(int lexical) {
  if (pSubParser) {
    if (pSubParser->handleLexical(lexical)) {
      delete pSubParser;
      pSubParser = 0;
    }
    return false;
  } else {
    if (lexical > 0) {
      handleCharacter(lexical);
      return false;
    } else {
      return handleToken(lexical);
    }
  }
}

void XmlParser::processChar(char c) {
  /*
   Analyse character to get next token.
   Analyser passes token to higher layer to process in turn, thus keeping higher layer away from detail.
   Analyser might pass two tokens if it needs the next character to decide the token type.
   */
  
  int lexical;
  
  pLexicalParser->processChar(c);
  while ((lexical = pLexicalParser->getNextToken()) != 0) {
      handleLexical(lexical);
  }
}

EntityParser::EntityParser() {
  state = STATE_IDLE;
}

char EntityParser::mapChar(char c) {
  switch (state) {
    case STATE_IDLE:
      if (c == '&') {
        index = 0;
        state = STATE_ENTITY;
        return 0;
      }
      return c;
      
    case STATE_ENTITY:
      if (c == ';') {
        state = STATE_IDLE;
        entity[index] = 0;
        if (!strcmp(entity, "quot")) {
          return '"';
        } else if (!strcmp(entity, "apos")) {
          return '\'';
        } else if (!strcmp(entity, "lt")) {
          return '<';
        } else if (!strcmp(entity, "gt")) {
          return '>';
        } else if (!strcmp(entity, "amp")) {
          return '&';
        } else {
          return 0;
        }
      }
      entity[index++] = c;
      return 0;
      
    default:
      return 0;
  }
}
