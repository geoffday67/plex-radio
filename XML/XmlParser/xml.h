#pragma once

#include "lexical.h"

#define MAX_NAME  256

typedef void (*XmlCallback)(char*, char*, void*);
typedef void (*CharCallback)(char*, char, void*);

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

  char name[MAX_NAME], value[MAX_NAME], entity[8], *pbuffer, attrName[MAX_NAME], attrValue[MAX_NAME];
  EntityParser *pEntityParser;
  LexicalParser *pLexicalParser;
  XmlParser *pSubParser;
  XmlCallback xmlCallback;
  CharCallback charCallback;
  bool ownLexical;
  void *pdata;
  int charIndex;
  
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
  void setData(void*);
  void reset();
};
