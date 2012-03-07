import module namespace csx="http://www.zorba-xquery.com/modules/csx";
import schema namespace foo="http://www.opencsx.org/schema";

declare variable $stream as xs:base64Binary := csx:serialize(validate{<foo:Employee><foo:Name>Luis</foo:Name><foo:Salary>10000000</foo:Salary></foo:Employee>}, "http://www.opencsx.org/vocab");
csx:parse($stream, "http://www.opencsx.org/vocab")
