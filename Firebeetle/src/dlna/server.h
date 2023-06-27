#pragma once

#include "../../XML/XmlParser/xml.h"
#include "object.h"

class BrowseResult {
 public:
  BrowseResult();
  ~BrowseResult();
  int count;
  Object *pObjects;

 private:
  void addObject(Object *);

  friend class DLNAServer;
};

class DLNAServer {
 private:
  BrowseResult *pBrowseResult;
  XmlParser browseParser, resultParser;
  static void browseCallback(char *, char *, void *);
  static void resultChar(char *, char, void *);
  static void resultCallback(char *, char *, void *);

 public:
  DLNAServer();

  char baseDomain[128];
  char descriptionURL[128];
  char controlPath[128];
  char id[128];
  char name[128];

  BrowseResult *browse(char *pid, int offset, int results);
};