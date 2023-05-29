//
//  lexical.h
//  XmlParser
//
//  Created by Geoff Day on 11/05/2023.
//

#ifndef lexical_h
#define lexical_h

class LexicalParser {
private:
  enum State {
    Idle,
    OpeningTag,
    Whitespace,
    Quoted,
    Escaped,
    EmptyTag,
  };
  State state;
  int tokens[8], tokenCount, tokenIndex;
  char quoteStart;
  
  void setTokens(int, ...);
  
public:
  static const int NO_TOKENS = 0;
  static const int START_OPEN_TAG = -1;
  static const int START_CLOSE_TAG = -2;
  static const int STOP_TAG = -3;
  static const int WHITESPACE = -4;
  static const int ATTRIBUTE_SEPARATOR = -5;
  static const int START_HEADER = -6;

  bool valueOnly = false;
  LexicalParser();
  
  // Returns 0 to indicate no more tokens available, or > 0 for a character, or < 0 for a non-character token.
  int getNextToken();
  void processChar(char);
};

#endif /* lexical_h */
