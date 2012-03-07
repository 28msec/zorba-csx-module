# Copyright 2012 Oracle.

# QQQ Due to https://bugs.launchpad.net/zorba/+bug/948879, these can't be
# marked TEST_ONLY as they really should be 
DECLARE_ZORBA_SCHEMA(FILE test.xsd URI http://www.opencsx.org/schema)
DECLARE_ZORBA_URI_FILE(FILE test.voc URI http://www.opencsx.org/vocab)
