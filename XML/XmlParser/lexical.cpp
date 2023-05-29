//
//  lexical.cpp
//  XmlParser
//
//  Created by Geoff Day on 11/05/2023.
//

#include <stdarg.h>
#include <ctype.h>
#include "lexical.h"

LexicalParser::LexicalParser() {
  state = State::Idle;
  tokenCount = 0;
  tokenIndex = 0;
  valueOnly = false;
}

void LexicalParser::setTokens(int count, ...) {
  int n;
  
  va_list args;
  va_start(args, count);
  for (n = 0; n < count; n++) {
    tokens[n] = va_arg(args, int);
  }
  va_end(args);
  
  tokenCount = count;
  tokenIndex = 0;
}

void LexicalParser::processChar(char c) {
  if (valueOnly) {
    if (c == '<') {
      state = State::OpeningTag;
      valueOnly = false;
    } else {
      setTokens(1, c);
    }
    return;
  }
  
  switch (state) {
      
    case State::Escaped:
      // Treat escaped character in quoted string as raw character.
      setTokens(1, c);
      state = State::Quoted;
      break;
      
    case State::Quoted:
      // Parsing for tags etc. doesn't apply here.
      if (c == quoteStart) {
        state = State::Idle;
      } else if (c == '\\') {
        state = State::Escaped;
      } else {
        setTokens(1, c);
      }
      break;
      
    case State::Whitespace:
      // Do nothing if this is another whitespace, process normally otherwise.
      if (isspace(c)) {
        break;
      }
      
    case State::Idle:
      if (c == '<') {
        state = State::OpeningTag;
      } else if (c == '>') {
        setTokens(1, STOP_TAG);
      } else if (isspace(c)) {
        setTokens(1, WHITESPACE);
        state = State::Whitespace;
      } else if (c == '=') {
        setTokens(1, ATTRIBUTE_SEPARATOR);
      } else if (c == '"' || c == '\'') {
        quoteStart = c;
        state = State::Quoted;
      } else if (c == '/') {
        state = State::EmptyTag;
      } else {
        setTokens(1, c);
      }
      break;
      
    case State::OpeningTag:
      if (c == '/') {
        setTokens(1, START_CLOSE_TAG);
      } else if (c == '?') {
        setTokens(1, START_HEADER);
      } else {
        setTokens(2, START_OPEN_TAG, c);
      }
      state = State::Idle;
      break;
      
    case State::EmptyTag:
      if (c == '>') {
        setTokens(3, STOP_TAG, START_CLOSE_TAG, STOP_TAG);
      } else {
        setTokens(2, '/', c);
      }
      state = State::Idle;
      break;
  }
}

int LexicalParser::getNextToken() {
  if (tokenIndex == tokenCount) {
    return NO_TOKENS;
  }
  
  return tokens[tokenIndex++];
}
