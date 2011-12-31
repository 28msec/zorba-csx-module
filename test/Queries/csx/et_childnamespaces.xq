import module namespace csx = "http://www.zorba-xquery.com/modules/csx";
declare variable $stream as xs:base64Binary :=
csx:serialize(<foo:xml xmlns:foo='http://www.opencsx.org'><foo:child xmlns:bar='http://www.opencsx.org/uri2' bar:zot='1'><bar:grandchild xmlns:baz='http://www.opencsx.org/uri3'><baz:greatgrandchild/></bar:grandchild><bar:grandchild/></foo:child></foo:xml>);
csx:parse($stream)