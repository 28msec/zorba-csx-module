import module namespace csx = "http://www.zorba-xquery.com/modules/csx";
declare variable $stream as xs:base64Binary := csx:serialize(<xml attr1='a' attr2='b'/>);
csx:parse($stream)