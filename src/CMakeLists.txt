FIND_PACKAGE ("OpenCSX")
INCLUDE_DIRECTORIES ("${OpenCSX_INCLUDE_DIR}")

DECLARE_ZORBA_MODULE (URI "http://www.zorba-xquery.com/modules/csx" 
  LINK_LIBRARIES "${OpenCSX_LIBRARY}" 
  VERSION 1.0 FILE "csx.xq"
)