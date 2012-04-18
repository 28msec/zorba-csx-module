xquery version "1.0";

module namespace csx = 'http://www.zorba-xquery.com/modules/csx';

declare namespace ver = "http://www.zorba-xquery.com/options/versioning";
declare option ver:module-version "1.0";

(:~
 : Translate from a CSX binary encoded stream to XML.
 :)
declare function csx:parse($csx as xs:string) as item()*
{
  csx:parse($csx, ())
};

(:~
 : Translate from a CSX binary encoded stream to XML, providing a URI to
 : an OpenCSX vocabulary file
 :)
declare function csx:parse($csxfile as xs:string, $vocab as xs:string*) as
  item()* external;

(:~
 : Translate from XML to a CSX binary stream.
 :)
declare function csx:serialize($xdm as item()*) as empty-sequence()
{
  csx:serialize($xdm, ())
};

(:~
 : Translate from XML to a CSX binary stream, providng a URI to an
 : OpenCSX vocabulary file
 :)
declare function csx:serialize($xdm as item()*, $vocab as xs:string*) as
  empty-sequence() external;
