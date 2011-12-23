import module namespace csx = "http://www.zorba-xquery.com/modules/csx";
declare variable $stream as xs:base64Binary := csx:serialize(<foo:xml xmlns:foo='bar'/>);
csx:parse($stream)