import module namespace csx = "http://www.zorba-xquery.com/modules/csx";
declare variable $stream as xs:base64Binary := csx:serialize
  (<foo:xml xmlns:foo='http://www.opencsx.org/uri1'
            xmlns:bar='http://www.opencsx.org/uri2' bar:zot='1'/>);
csx:parse($stream)