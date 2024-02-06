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

  char baseDomain[URL_SIZE];
  char descriptionURL[URL_SIZE];
  char controlPath[URL_SIZE];
  char id[ID_SIZE];
  char name[NAME_SIZE];

  void browse(char *pid, int offset, int results, char *pfilter, int *pfound, ObjectCallback objectCallback);
};