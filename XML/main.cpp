#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "xml.h"

XmlParser parser, resultParser;
char basicInput[] = R"(
<?xml version="1.0" encoding="UTF-8"?>
<s:Envelope s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"><s:Body><u:BrowseResponse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1"><Result>&lt;DIDL-Lite xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/" xmlns:dlna="urn:schemas-dlna-org:metadata-1-0/"&gt;&lt;item id="fa4061f7aef9e7215a07" parentID="ab31bafbbb287b8e9bb9" restricted="1"&gt;&lt;dc:title&gt;Sweet Lorraine&lt;/dc:title&gt;&lt;dc:creator&gt;Nat King Cole&lt;/dc:creator&gt;&lt;dc:date&gt;2004-01-01&lt;/dc:date&gt;&lt;upnp:artist&gt;Nat King Cole&lt;/upnp:artist&gt;&lt;upnp:album&gt;20 Golden Greats&lt;/upnp:album&gt;&lt;upnp:genre&gt;Unknown&lt;/upnp:genre&gt;&lt;upnp:albumArtURI dlna:profileID="JPEG_MED"&gt;http://192.168.68.106:32469/proxy/a8a415f5feabf1f86d8e/albumart.jpg&lt;/upnp:albumArtURI&gt;&lt;dc:description&gt;Sweet Lorraine&lt;/dc:description&gt;&lt;upnp:icon&gt;http://192.168.68.106:32469/proxy/5600f205fededc1d9adf/icon.jpg&lt;/upnp:icon&gt;&lt;upnp:originalTrackNumber&gt;1&lt;/upnp:originalTrackNumber&gt;&lt;res duration="0:04:37.000" bitrate="85750" sampleFrequency="44100" nrAudioChannels="2" protocolInfo="http-get:*:audio/flac:DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"&gt;http://192.168.68.106:32469/object/fa4061f7aef9e7215a07/file.flac&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id="25c5b27880aae2a04eed" parentID="ab31bafbbb287b8e9bb9" restricted="1"&gt;&lt;dc:title&gt;Straighten Up and Fly Right&lt;/dc:title&gt;&lt;dc:creator&gt;Nat King Cole&lt;/dc:creator&gt;&lt;dc:date&gt;2004-01-01&lt;/dc:date&gt;&lt;upnp:artist&gt;Nat King Cole&lt;/upnp:artist&gt;&lt;upnp:album&gt;20 Golden Greats&lt;/upnp:album&gt;&lt;upnp:genre&gt;Unknown&lt;/upnp:genre&gt;&lt;upnp:albumArtURI dlna:profileID="JPEG_MED"&gt;http://192.168.68.106:32469/proxy/a8a415f5feabf1f86d8e/albumart.jpg&lt;/upnp:albumArtURI&gt;&lt;dc:description&gt;Straighten Up and Fly Right&lt;/dc:description&gt;&lt;upnp:icon&gt;http://192.168.68.106:32469/proxy/5600f205fededc1d9adf/icon.jpg&lt;/upnp:icon&gt;&lt;upnp:originalTrackNumber&gt;2&lt;/upnp:originalTrackNumber&gt;&lt;res duration="0:02:38.000" bitrate="77500" sampleFrequency="44100" nrAudioChannels="2" protocolInfo="http-get:*:audio/flac:DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"&gt;http://192.168.68.106:32469/object/25c5b27880aae2a04eed/file.flac&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id="cfc36f028b73760e4bad" parentID="ab31bafbbb287b8e9bb9" restricted="1"&gt;&lt;dc:title&gt;Nature Boy&lt;/dc:title&gt;&lt;dc:creator&gt;Nat King Cole&lt;/dc:creator&gt;&lt;dc:date&gt;2004-01-01&lt;/dc:date&gt;&lt;upnp:artist&gt;Nat King Cole&lt;/upnp:artist&gt;&lt;upnp:album&gt;20 Golden Greats&lt;/upnp:album&gt;&lt;upnp:genre&gt;Unknown&lt;/upnp:genre&gt;&lt;upnp:albumArtURI dlna:profileID="JPEG_MED"&gt;http://192.168.68.106:32469/proxy/a8a415f5feabf1f86d8e/albumart.jpg&lt;/upnp:albumArtURI&gt;&lt;dc:description&gt;Nature Boy&lt;/dc:description&gt;&lt;upnp:icon&gt;http://192.168.68.106:32469/proxy/5600f205fededc1d9adf/icon.jpg&lt;/upnp:icon&gt;&lt;upnp:originalTrackNumber&gt;3&lt;/upnp:originalTrackNumber&gt;&lt;res duration="0:02:55.000" bitrate="85875" sampleFrequency="44100" nrAudioChannels="2" protocolInfo="http-get:*:audio/flac:DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"&gt;http://192.168.68.106:32469/object/cfc36f028b73760e4bad/file.flac&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id="c15d47205f06f2b5b874" parentID="ab31bafbbb287b8e9bb9" restricted="1"&gt;&lt;dc:title&gt;Dance Ballerina Dance&lt;/dc:title&gt;&lt;dc:creator&gt;Nat King Cole&lt;/dc:creator&gt;&lt;dc:date&gt;2004-01-01&lt;/dc:date&gt;&lt;upnp:artist&gt;Nat King Cole&lt;/upnp:artist&gt;&lt;upnp:album&gt;20 Golden Greats&lt;/upnp:album&gt;&lt;upnp:genre&gt;Unknown&lt;/upnp:genre&gt;&lt;upnp:albumArtURI dlna:profileID="JPEG_MED"&gt;http://192.168.68.106:32469/proxy/a8a415f5feabf1f86d8e/albumart.jpg&lt;/upnp:albumArtURI&gt;&lt;dc:description&gt;Dance Ballerina Dance&lt;/dc:description&gt;&lt;upnp:icon&gt;http://192.168.68.106:32469/proxy/5600f205fededc1d9adf/icon.jpg&lt;/upnp:icon&gt;&lt;upnp:originalTrackNumber&gt;4&lt;/upnp:originalTrackNumber&gt;&lt;res duration="0:02:42.000" bitrate="105750" sampleFrequency="44100" nrAudioChannels="2" protocolInfo="http-get:*:audio/flac:DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"&gt;http://192.168.68.106:32469/object/c15d47205f06f2b5b874/file.flac&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;item id="4b0de6f3d4d7d8106349" parentID="ab31bafbbb287b8e9bb9" restricted="1"&gt;&lt;dc:title&gt;Mona Lisa&lt;/dc:title&gt;&lt;dc:creator&gt;Nat King Cole&lt;/dc:creator&gt;&lt;dc:date&gt;2004-01-01&lt;/dc:date&gt;&lt;upnp:artist&gt;Nat King Cole&lt;/upnp:artist&gt;&lt;upnp:album&gt;20 Golden Greats&lt;/upnp:album&gt;&lt;upnp:genre&gt;Unknown&lt;/upnp:genre&gt;&lt;upnp:albumArtURI dlna:profileID="JPEG_MED"&gt;http://192.168.68.106:32469/proxy/a8a415f5feabf1f86d8e/albumart.jpg&lt;/upnp:albumArtURI&gt;&lt;dc:description&gt;Mona Lisa&lt;/dc:description&gt;&lt;upnp:icon&gt;http://192.168.68.106:32469/proxy/5600f205fededc1d9adf/icon.jpg&lt;/upnp:icon&gt;&lt;upnp:originalTrackNumber&gt;5&lt;/upnp:originalTrackNumber&gt;&lt;res duration="0:03:28.000" bitrate="87750" sampleFrequency="44100" nrAudioChannels="2" protocolInfo="http-get:*:audio/flac:DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01500000000000000000000000000000"&gt;http://192.168.68.106:32469/object/4b0de6f3d4d7d8106349/file.flac&lt;/res&gt;&lt;upnp:class&gt;object.item.audioItem.musicTrack&lt;/upnp:class&gt;&lt;/item&gt;&lt;/DIDL-Lite&gt;</Result><NumberReturned>5</NumberReturned><TotalMatches>20</TotalMatches><UpdateID>84263588</UpdateID></u:BrowseResponse></s:Body></s:Envelope>
)";
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

void charCallback(char *pname, char c) {
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

void resultCallback(char *pname, char *pvalue) {
  if (pvalue) {
    printf("%s = %s\n", pname, pvalue);
  //} else {
    //printf("Result, %s starting\n", pname);
  }
}

void testCharCallback(char *pname, char value) {
  resultParser.processChar(value);
}

void testCallback(char *pname, char *pvalue) {
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
    parser.processChar(basicInput[n]);
  }
  
  printf("Success!\n");
  return 0;
}

