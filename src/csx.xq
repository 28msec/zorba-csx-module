xquery version "3.0";

module namespace csx = 'http://www.zorba-xquery.com/modules/csx';

declare namespace ver = "http://www.zorba-xquery.com/options/versioning";
declare option ver:module-version "1.0";

(:~
 : Translate from a CSX binary encoded stream to a XML
 :)
declare function csx:parse($csx as xs:base64Binary) as 
  item()* external;

(:~
 : Translate from a XML stream to a CSX binary stream
 :)
declare function csx:serialize($xdm as item()*) as
  xs:base64Binary external;
  