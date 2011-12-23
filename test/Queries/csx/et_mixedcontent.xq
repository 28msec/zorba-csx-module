import module namespace csx = "http://www.zorba-xquery.com/modules/csx";
declare variable $stream as xs:base64Binary := csx:serialize(<xml>chars<child>hi</child>more</xml>);
csx:parse($stream)