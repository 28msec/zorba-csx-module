import module namespace csx = "http://www.zorba-xquery.com/modules/csx";
declare variable $stream as xs:base64Binary := csx:serialize(<xml xmlns='http://www.opencsx.org'/>);
csx:parse($stream)