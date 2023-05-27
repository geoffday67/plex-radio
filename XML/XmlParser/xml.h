#pragma once

#include "lexical.h"

typedef void (*XmlCallback)(char const*, char const*);
typedef void (*CharCallback)(char const*, const char);

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
  void reset();
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

  char name[128], value[128], entity[8], *pbuffer, attrName[128], attrValue[128];
  EntityParser entityParser;
  LexicalParser *pLexicalParser;
  XmlParser *pSubParser;
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

public:
  XmlParser();
  XmlParser(LexicalParser*);
  ~XmlParser();
  void processChar(char);
  XmlCallback xmlCallback;
  CharCallback charCallback;
};
