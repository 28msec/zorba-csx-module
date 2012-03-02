#include <zorba/item_factory.h>
#include <zorba/singleton_item_sequence.h>
#include <zorba/vector_item_sequence.h>
#include <zorba/item.h>
#include <zorba/diagnostic_list.h>
#include <zorba/empty_sequence.h>
#include <zorba/store_manager.h>
#include <zorba/zorba_exception.h>
#include <zorba/store_consts.h>
#include <zorba/base64.h>

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "csx.h"

namespace zorba { namespace csx {

  using namespace std;

  const String CSXModule::CSX_MODULE_NAMESPACE = "http://www.zorba-xquery.com/modules/csx";

  /*******************************************************************************************
  *******************************************************************************************/
  
  zorba::ExternalFunction*
  CSXModule::getExternalFunction(const zorba::String& localName) {
    if(localName == "parse"){
      if(!theParseFunction){
        theParseFunction = new ParseFunction(this);
      } 
      return theParseFunction;
    } else if(localName == "serialize"){
      if(!theSerializeFunction){
        theSerializeFunction = new SerializeFunction(this);
      }
      return theSerializeFunction;
    }
    return NULL;
  }

  void CSXModule::destroy() 
  {
    delete this;
  }

  void CSXModule::loadVocab(opencsx::CSXProcessor *aProc, Iterator_t aUriIter) const
  {
    aUriIter->open();
    Item aUri;
    while (aUriIter->next(aUri)) {
      void *lStore = zorba::StoreManager::getStore();
      Zorba* lZorba = Zorba::getInstance(lStore);
      Item vocab_item = lZorba->getXmlDataManager()->fetch(aUri.getStringValue());
      aProc->loadVocabulary(vocab_item.getStream());
    }
    aUriIter->close();
  }

  CSXModule::~CSXModule()
  {
    if(!theParseFunction)
      delete theParseFunction;
    if(!theSerializeFunction)
      delete theSerializeFunction;
  }

  /*******************************************************************************************
  *******************************************************************************************/

  string getKindAsString(int kind){
    string sKind = "Unknown";
    switch(kind){
      case zorba::store::StoreConsts::attributeNode:
        sKind = "Attribute";
        break;
      case zorba::store::StoreConsts::commentNode:
        sKind = "Comment";
        break;
      case zorba::store::StoreConsts::documentNode:
        sKind = "Document";
        break;
      case zorba::store::StoreConsts::elementNode:
        sKind = "Element";
        break;
      case zorba::store::StoreConsts::textNode:
        sKind = "Text";
        break;
      case zorba::store::StoreConsts::piNode:
        sKind = "Pi";
        break;
      case zorba::store::StoreConsts::anyNode:
        sKind = "Any";
        break;
    }
    return sKind;
  }

  void getTypedData(Item item, opencsx::AtomicValue *v){
    store::SchemaTypeCode iType = item.getTypeCode();
    //cout << "Item Type: ";
    switch(iType){
    case store::XS_BOOLEAN:
      cout << "boolean";
      v->m_type = opencsx::DT_BOOLEAN;
      v->m_value.f_bool = item.getBooleanValue();
      break;
    case store::XS_BYTE:
      cout << "byte";
      v->m_type = opencsx::DT_BYTE;
      v->m_value.f_char = (unsigned char)item.getUnsignedIntValue();
      break;
    case store::XS_INT:
      //cout << "integer";
      v->m_type = opencsx::DT_INT;
      v->m_value.f_int = item.getIntValue();
      break;
    case store::XS_LONG:
      cout << "long";
      v->m_type = opencsx::DT_LONG;
      v->m_value.f_long = item.getLongValue();
      break;
    case store::XS_FLOAT:
      cout << "float";
      v->m_type = opencsx::DT_FLOAT;
      v->m_value.f_float = (float)item.getDoubleValue();
      break;
    case store::XS_DOUBLE:
      cout << "double";
      v->m_type = opencsx::DT_DOUBLE;
      v->m_value.f_double = item.getDoubleValue();
      break;
    case store::XS_STRING:
    case store::XS_ANY_ATOMIC:
    default:
      v->m_type = opencsx::DT_STRING;
      v->m_string = item.getStringValue().str();
      break;
    }
  }

  bool traverse(Iterator_t iter, opencsx::CSXHandler* handler, bool skip_text){
    // given a iterator transverse the item()* tree
    Item name, item, attr, aName;
    Iterator_t children, attrs;
    opencsx::AtomicValue v;
    bool saw_atomics = false;
    // here comes the magic :)
    iter->open();
    while(iter->next(item)){
      if(item.isNode()){
        int kind = item.getNodeKind();
        //cout << "node type: " << getKindAsString(kind) << endl;
        if(kind == zorba::store::StoreConsts::elementNode){
          // Map local namespace bindings to CSXHandler form
          zorba::NsBindings bindings;
          item.getNamespaceBindings(bindings,
                                    zorba::store::StoreConsts::ONLY_LOCAL_NAMESPACES);
          zorba::NsBindings::const_iterator ite = bindings.begin();
          zorba::NsBindings::const_iterator end = bindings.end();
          opencsx::CSXHandler::NsBindings csxbindings;
          for (; ite != end; ++ite) {
            csxbindings.push_back(std::pair<std::string,std::string>
                                  (ite->first.str(), ite->second.str()));
          }

          //cout << "<StartElement>" << endl;
          item.getNodeName(name);
          string ns, pr, ln;
          //cout << "name: " << name.getNamespace() << "::" << name.getPrefix() << "::" << name.getLocalName() << endl;
          ns = name.getNamespace().str();
          pr = name.getPrefix().str();
          ln = name.getLocalName().str();
          //cout << "str name: " << ns << "::" << pr << "::" << ln << endl;
          handler->startElement(ns, ln, pr, &csxbindings);

          // go thru attributes
          attrs = item.getAttributes();
          if(!attrs.isNull()){
            attrs->open();
            while(attrs->next(attr)){
              kind = attr.getNodeKind();
              //cout << "attr type: " << getKindAsString(kind) << endl;
              attr.getNodeName(aName);
              //cout << "attr name: " << aName.getNamespace() << "::" << aName.getPrefix() << "::" << aName.getLocalName() << endl;
              //cout << "attr text: " << attr.getStringValue() << endl;
              getTypedData(attr,&v);
              handler->attribute(aName.getNamespace().str(), aName.getLocalName().str(),
                                 aName.getPrefix().str(), v);
            }
          }

          // QQQ Hack necessary at the moment - only way to get typed value is
          // via getAtomizationValue() (poorly named), but it also throws an
          // exception if there is element-only content.
          bool has_atomic_values = false;
          try {
            children = item.getAtomizationValue();
            has_atomic_values = traverse(children, handler, false);
          }
          catch (ZorbaException &e) {
            //cout << e << endl;
          }
          children = item.getChildren();
          if(!children.isNull()){
            // there is at least one child. We want to skip any text node children
            // if we've already processing atomic-value children.
            traverse(children, handler, has_atomic_values);
          }

          //cout << "<EndElement>" << endl;
          handler->endElement(name.getNamespace().str(), name.getLocalName().str(),
                              name.getPrefix().str());
        }
        else if (kind == zorba::store::StoreConsts::textNode) {
          //cout << "<Characters>" << endl;
          if (!skip_text) {
            handler->characters(item.getStringValue().str());
          }
          children = NULL;
          //cout << "text content: " << item.getStringValue() << endl;
        }
        else {
          assert(false);
        }
      }
      else {
        opencsx::AtomicValue v;
        getTypedData(item, &v);
        handler->atomicValue(v);
        saw_atomics = true;
      }
    }
    // here ends the magic :(
    iter->close();

    return saw_atomics;
  }

  zorba::ItemSequence_t
    SerializeFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    // Second argument is URIs to vocab files (optional)
    theModule->loadVocab(theProcessor, aArgs[1]->getIterator());

    stringstream theOutputStream;
    opencsx::CSXHandler* csxHandler = theProcessor->createSerializer(theOutputStream);

    // OpenCSX requires a document to create a CSX section header; might be a bug
    csxHandler->startDocument();

    try {
      // First arg is the item* to serialize
      traverse(aArgs[0]->getIterator(), csxHandler, false);
    } catch(ZorbaException ze){
      cerr << ze << endl;
    }

    csxHandler->endDocument();

    string base64Result = zorba::encoding::Base64::encode(theOutputStream.str()).str();
    return ItemSequence_t(
      new SingletonItemSequence(Zorba::getInstance(0)->getItemFactory()->createBase64Binary(base64Result.c_str(),base64Result.length()))
    );
  }


/*******************************************************************************************
  *******************************************************************************************/
  zorba::ItemSequence_t
    ParseFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    // Second arg is URIs to vocab files (optional)
    theModule->loadVocab(theProcessor, aArgs[1]->getIterator());

    vector<Item> v;
    auto_ptr<CSXParserHandler> parserHandler(new CSXParserHandler(v));

    try {
      // First arg is the base64 item to parse
      Iterator_t iter = aArgs[0]->getIterator();
      iter->open();
      Item input;
      if(iter->next(input)){
        string iString = zorba::encoding::Base64::decode(input.getStringValue()).str();
        stringstream ss(iString);
        parserHandler->startDocument();
        theProcessor->parse(ss, parserHandler.get());
        input.close();
      }
      iter->close();
    }
    catch(ZorbaException ze){
      cerr << ze << endl;
    }

    VectorItemSequence *vSeq = (v.size() > 0)?new VectorItemSequence(v):NULL;
    parserHandler->endDocument();
    return ItemSequence_t(vSeq);
  }

  /*** Start the CSXParserHandler implementation ***/

  CSXParserHandler::CSXParserHandler(vector<Item>& aItems)
    : m_result(aItems) {
    m_defaultType = Zorba::getInstance(0)->getItemFactory()->createQName(zorba::String("http://www.w3.org/2001/XMLSchema"),zorba::String("untyped"));
    m_defaultAttrType = Zorba::getInstance(0)->getItemFactory()->createQName(zorba::String("http://www.w3.org/2001/XMLSchema"),zorba::String("AnyAtomicType"));
  }

  void CSXParserHandler::startDocument(){
  }

  void CSXParserHandler::endDocument(){
    m_itemStack.clear();
  }

  void printVector(vector<pair<zorba::String, zorba::String> > v){
    cout << "[";
    for(vector<pair<zorba::String, zorba::String> >::iterator it = v.begin(); it != v.end(); it++){
      cout << "[" << it->first << ", " << it->second << "]" << endl;
    }
    cout << "]" << endl;
  }

  void CSXParserHandler::startElement(const string& uri, const string& localname,
                                      const string& prefix,
                                      const opencsx::CSXHandler::NsBindings *bindings){
    zorba::String zUri = zorba::String(uri);
    zorba::String zLocalName = zorba::String(localname);
    zorba::String zPrefix = zorba::String(prefix);
    Zorba* z = Zorba::getInstance(0);
    //cout << "nodename composites: zUri: " << zUri << " zPrefix: " << zPrefix << " zLocalName: " << zLocalName << endl;
    Item nodeName = z->getItemFactory()->createQName(zUri, zPrefix, zLocalName);

    opencsx::CSXHandler::NsBindings::const_iterator ite = bindings->begin();
    opencsx::CSXHandler::NsBindings::const_iterator end = bindings->end();
    zorba::NsBindings itembindings;
    for (; ite != end; ++ite) {
      itembindings.push_back(pair<zorba::String,zorba::String>
                             (zorba::String(ite->first), zorba::String(ite->second)));
    }

    Item parent = m_itemStack.empty() ? Item() : m_itemStack.back();
    Item thisNode;
    thisNode = z->getItemFactory()->createElementNode(parent, nodeName,
                                                      m_defaultType, false, false,
                                                      itembindings);
    m_itemStack.push_back(thisNode);
  }

  void CSXParserHandler::endElement(const string &uri, const string &localname, const string &prefix){
    // Pop current element off stack; if it's unparented, add it to the output vector
    // QQQ We don't currently handle anything other than elements as results
    Item lItem = m_itemStack.back();
    m_itemStack.pop_back();
    if (lItem.getParent().isNull()) {
      m_result.push_back(lItem);
    }
  }

  void CSXParserHandler::characters(const string &chars){
    // create text node
    Zorba::getInstance(0)->getItemFactory()->createTextNode(m_itemStack.back(), chars);
  }

  void CSXParserHandler::atomicValue(const opencsx::AtomicValue &value) {
    // QQQ unimplemented
    assert(false);
  }

  void CSXParserHandler::attribute(const string &uri, const string &localname,
                                   const string &prefix, const string &value){
    zorba::String zAttrName = zorba::String(localname);
    zorba::String zUri = zorba::String(uri);
    zorba::String zPrefix = zorba::String(prefix);
    Zorba* z = Zorba::getInstance(0);
    Item nodeName = z->getItemFactory()->createQName(zUri, zPrefix, zAttrName);
    Item attrNodeValue = z->getItemFactory()->createString(zorba::String(value));
    z->getItemFactory()->createAttributeNode(m_itemStack.back(), nodeName, m_defaultAttrType, attrNodeValue);
  }

  void CSXParserHandler::attribute(const string &uri, const string &localname,
                                   const string &qname, const opencsx::AtomicValue &value) {
    // QQQ unimplemented
    assert(false);
  }

  void CSXParserHandler::processingInstruction(const string &target, const string &data){
    cout << "inside processinginstruction(" << target << ", " << data << ")" << endl;
    // No need to implement
  }

  void CSXParserHandler::comment(const string &chars){
    cout << "inside comment(" << chars << ")" << endl;
    // No need to implement
  }

  CSXParserHandler::~CSXParserHandler(){
    m_itemStack.clear();
  }
 
}/*namespace csx*/ }/*namespace zorba*/

#ifdef WIN32
#  define CSX_DLL_EXPORT __declspec(dllexport)
#else
#  define CSX_DLL_EXPORT __attribute__ ((visibility("default")))
#endif

extern "C" CSX_DLL_EXPORT zorba::ExternalModule* createModule() {
  return new zorba::csx::CSXModule();
}
