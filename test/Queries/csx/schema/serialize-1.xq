import module namespace csx="http://www.zorba-xquery.com/modules/csx";
import schema namespace foo="http://www.opencsx.org/schema";
declare namespace bar="http://www.opencsx.org/schema";

csx:serialize(validate { <bar:Employee><bar:Name>Luis</bar:Name><bar:Salary>10000000</bar:Salary></bar:Employee> }, ("http://www.opencsx.org/vocab"))


