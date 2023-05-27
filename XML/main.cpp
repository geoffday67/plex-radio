#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "xml.h"

XmlParser parser;
char basicInput[] = R"(
<abc>def</abc>
<abc geoff='He\'s great' jeanette="She's lovely">
<sub1>
<sub11>def</sub11>
</sub1>
<sub2>ghi</sub2>
</abc>
<Result>&lt;gd&gt;</Result>
<&quot;&apos;&amp;ghi>jkl</ghi>
<top><middle>second<bottom>embedded</bottom></middle></top>
<ns:empty/>
)";

void charCallback(char const *pname, const char c) {
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
      parser.charCallback = 0;
      cycle = 0;
      break;
    default:
      break;
  }
}

void basicCallback(char const *pname, char const *pvalue) {
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
      parser.charCallback = charCallback;
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

void testCharCallback(char const *pname, char const value) {
  printf("%s char = %c\n", pname, value);
}

void testCallback(char const *pname, char const *pvalue) {
  if (pvalue) {
    printf("%s ending, value = %s\n", pname, pvalue);
  } else {
    printf("%s starting\n", pname);
  }
  if (!strcmp(pname, "Result")) {
    if (!pvalue) {
      parser.charCallback = testCharCallback;
    } else {
      parser.charCallback = 0;
    }
  }
}

int main() {
  parser.xmlCallback = testCallback;
  for (int n = 0; n < strlen(basicInput); n++) {
    parser.processChar(basicInput[n]);
  }
  
  printf("Success!\n");
  return 0;
}

