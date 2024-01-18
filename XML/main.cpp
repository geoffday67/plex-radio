#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "xml.h"

XmlParser parser, resultParser;
char basicInput[] = R"(<Result>&lt;dc:title&gt;Motörhead - The best of Motörhead. Deaf forever (1998)&lt;/dc:title&gt;</Result>)";
/*
<abc>def</abc>
<abc geoff='He\'s great' jeanette="She's lovely">
<sub1>
<sub11>def</sub11>
</sub1>
<sub2>ghi</sub2>
</abc>
<Envelope><Result>&lt;gdxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx&gt;</Result></Envelope>
<&quot;&apos;&amp;ghi>jkl</ghi>
<top><middle>second<bottom>embedded</bottom></middle></top>
<ns:empty/>
*/

void charCallback(char *pname, char c, void *pdata) {
  static int cycle = 1;
  
  assert(!strcmp(pname, "Result"));
  switch (cycle) {
    case 1:
      assert(c == '<');
      cycle = 2;
      break;
    case 2:
      assert(c == 'g');
      cycle = 3;
      break;
    case 3:
      assert(c == 'd');
      cycle = 4;
      break;
    case 4:
      assert(c == '>');
      cycle = 5;
      break;
    case 5:
      assert(c == 0);
      parser.setCharCallback(0);
      cycle = 0;
      break;
    default:
      break;
  }
}

void basicCallback(char *pname, char *pvalue) {
  static int cycle = 1;
  
  switch (cycle) {
    case 0:
      throw "Extra callback received";
    case 1:
      assert(!strcmp(pname, "abc"));
      assert(pvalue == NULL);
      cycle = 2;
      break;
    case 2:
      assert(!strcmp(pname, "abc"));
      assert(!strcmp(pvalue, "def"));
      cycle = 0;
      break;
    case 3:
      assert(!strcmp(pname, "Result"));
      assert(pvalue == NULL);
      parser.setCharCallback(charCallback);
      cycle = 4;
      break;
    case 4:
      assert(!strcmp(pname, "\"'&ghi"));
      assert(pvalue == NULL);
      cycle = 5;
      break;
    case 5:
      assert(!strcmp(pname, "\"'&ghi"));
      assert(!strcmp(pvalue, "jkl"));
      cycle = 0;
      break;
    default:
      break;
  }
}

void resultCallback(char *pname, char *pvalue, void *pdata) {
  if (!strcmp(pname, "dc:title") && pvalue) {
    printf("%s = %s\n", pname, pvalue);
  //} else {
    //printf("Result, %s starting\n", pname);
  }
}

void testCharCallback(char *pname, char value, void *pdata) {
  resultParser.processChar(value);
}

void testCallback(char *pname, char *pvalue, void *pdata) {
  if (pvalue && *pvalue) {
    printf("%s = %s\n", pname, pvalue);
  //} else {
    //printf("%s starting\n", pname);
  }
  if (!strcmp(pname, "Result")) {
    if (!pvalue) {
      parser.setCharCallback(testCharCallback);
      resultParser.reset();
    } else {
      parser.setCharCallback(0);
    }
  }
}

int main() {
  parser.setXmlCallback(testCallback);
  resultParser.setXmlCallback(resultCallback);
  for (int n = 0; n < strlen(basicInput); n++) {
    printf("%04X[%c] ", basicInput[n], basicInput[n]);
    parser.processChar(basicInput[n]);
  }
  
  printf("Success!\n");
  return 0;
}

