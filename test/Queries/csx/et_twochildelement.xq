import module namespace csx = "http://www.zorba-xquery.com/modules/csx";
declare variable $stream as xs:base64Binary := csx:serialize(<xml><child/><child2/></xml>);
csx:parse($stream)