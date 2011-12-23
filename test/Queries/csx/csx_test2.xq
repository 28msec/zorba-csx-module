import module namespace csx = "http://www.zorba-xquery.com/modules/csx";
declare variable $stream as xs:base64Binary := csx:serialize(<entry author="Fix" date="2011-10-13-07:00" time="14:43:55.655-07:00">Foxy</entry>);
csx:parse($stream)
