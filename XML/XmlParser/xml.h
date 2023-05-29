#pragma once

#include "lexical.h"

typedef void (*XmlCallback)(char*, char*);
typedef void (*CharCallback)(char*, char);

class EntityParser {
private:
  static const int STATE_IDLE = 0;
  static const int STATE_ENTITY = 1;
  
  char entity[8];
  int state, index;
  
  void log(char const*);
  void handleIdle(char);
  void handleEntity(char);
  
public:
  EntityParser();
  char mapChar(char);
};

class XmlParser {
private:
  enum State {
    Idle,
    TagName,
    AttrName,
    AttrValue,
    Body,
    TagClose,
  };
  State state;

  char name[256], value[256], entity[8], *pbuffer, attrName[256], attrValue[256];
  EntityParser *pEntityParser;
  LexicalParser *pLexicalParser;
  XmlParser *pSubParser;
  XmlCallback xmlCallback;
  CharCallback charCallback;
  bool ownLexical;
  
  void log(char const*);
  bool handleLexical(int);  // TRUE => end of tag parsing.
  bool handleToken(int);    // TRUE => end of tag parsing.
  void handleCharacter(char);
  void startTagName();
  void endTagName();
  void startAttrName();
  void endAttrName();
  void startAttrValue();
  void endAttrValue();
  void startBody();
  void endBody();
  void startSubParser();
  void endSubParser();
  char *stripNamespace(char*);
  void init();

public:
  XmlParser();
  XmlParser(LexicalParser*);
  ~XmlParser();
  void processChar(char);
  void setXmlCallback(XmlCallback);
  void setCharCallback(CharCallback);
  void reset();
};
