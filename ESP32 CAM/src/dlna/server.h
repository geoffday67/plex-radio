#pragma once

#include "../../XML/XmlParser/xml.h"
#include "object.h"

typedef void (*ObjectCallback)(Object *);

class DLNAServer {
 private:
  XmlParser *pBrowseParser, *pResultParser;
  ObjectCallback objectCallback;
  int found;

  static void browseCallback(char *, char *, void *);
  static void resultChar(char *, char, void *);
  static void resultCallback(char *, char *, void *);

 public:
  DLNAServer();
  DLNAServer(DLNAServer&);
  ~DLNAServer();

  char baseDomain[128];
  char descriptionURL[128];
  char controlPath[128];
  char id[128];
  char name[128];

  void browse(char *pid, int offset, int results, char *pfilter, int *pfound, ObjectCallback objectCallback);
};