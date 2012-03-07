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
              Iterator_t values = attr.getAtomizationValue();
              values->open();
              Item value;
              // QQQ Since there's no way to pass multiple AtomicValues for an
              // attribute to OpenCSX, we just get the first.
              values->next(value);
              getTypedData(value,&v);
              handler->attribute(aName.getNamespace().str(), aName.getLocalName().str(),
                                 aName.getPrefix().str(), v);
              values->close();
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
            opencsx::AtomicValue v;
            v.m_type = opencsx::DT_ANYATOMIC;
            v.m_string = item.getStringValue().str();
            handler->atomicValue(v);
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
    m_itemFactory = Zorba::getInstance(NULL)->getItemFactory();
    m_defaultType = m_itemFactory->createQName(
          zorba::String("http://www.w3.org/2001/XMLSchema"),
          zorba::String("untyped"));
    m_defaultAttrType = m_itemFactory->createQName(
          zorba::String("http://www.w3.org/2001/XMLSchema"),
          zorba::String("AnyAtomicType"));
  }

  void CSXParserHandler::startDocument(){
  }

  void CSXParserHandler::endDocument(){
    m_elemStack.clear();
  }

  void CSXParserHandler::startElement(const string& uri, const string& localname,
                                      const string& prefix,
                                      const opencsx::CSXHandler::NsBindings *bindings){
    zorba::String zUri = zorba::String(uri);
    zorba::String zLocalName = zorba::String(localname);
    zorba::String zPrefix = zorba::String(prefix);
    //cout << "nodename composites: zUri: " << zUri << " zPrefix: " << zPrefix << " zLocalName: " << zLocalName << endl;
    Item nodeName = m_itemFactory->createQName(zUri, zPrefix, zLocalName);

    opencsx::CSXHandler::NsBindings::const_iterator ite = bindings->begin();
    opencsx::CSXHandler::NsBindings::const_iterator end = bindings->end();
    zorba::NsBindings itembindings;
    for (; ite != end; ++ite) {
      itembindings.push_back(pair<zorba::String,zorba::String>
                             (zorba::String(ite->first), zorba::String(ite->second)));
    }

    Item parent = m_elemStack.empty() ? Item() : m_elemStack.back();
    Item thisNode;
    thisNode = m_itemFactory->createElementNode(parent, nodeName,
                                                      m_defaultType, true, false,
                                                      itembindings);
    m_elemStack.push_back(thisNode);
  }

  void CSXParserHandler::endElement(const string &uri, const string &localname, const string &prefix){
    // If there are cached atomics, assign them as the typed-value of the
    // current element.
    Item lItem = m_elemStack.back();
    if (m_atomics.size() > 0) {
      m_itemFactory->assignElementTypedValue(lItem, m_atomics);
      m_atomics.clear();
    }

    // Pop current element off stack; if it's unparented, add it to the output vector
    // QQQ We don't currently handle anything other than elements as results
    m_elemStack.pop_back();
    if (lItem.getParent().isNull()) {
      m_result.push_back(lItem);
    }
  }

  void CSXParserHandler::atomicValue(const opencsx::AtomicValue &value) {
    // AnyAtomic strings are text nodes in Zorba.
    if (value.m_type == opencsx::DT_ANYATOMIC) {
      m_itemFactory->createTextNode(m_elemStack.back(), value.m_string);
    }
    else {
      // Cache atomics in case the element has a list of them.
      m_atomics.push_back(getAtomicItem(value));
    }
  }

  void CSXParserHandler::attribute(const string &uri, const string &localname,
                                   const string &prefix, const opencsx::AtomicValue &value) {
    zorba::String zAttrName = zorba::String(localname);
    zorba::String zUri = zorba::String(uri);
    zorba::String zPrefix = zorba::String(prefix);
    Item nodeName = m_itemFactory->createQName(zUri, zPrefix, zAttrName);
    // QQQ handle other simple types!
    Item attrNodeValue = m_itemFactory->createString(zorba::String(value.m_string));
    m_itemFactory->createAttributeNode(
          m_elemStack.back(), nodeName, m_defaultAttrType, attrNodeValue);
  }

  void CSXParserHandler::processingInstruction(const string &target, const string &data){
    cout << "inside processinginstruction(" << target << ", " << data << ")" << endl;
    // No need to implement yet
  }

  void CSXParserHandler::comment(const string &chars){
    cout << "inside comment(" << chars << ")" << endl;
    // No need to implement yet
  }

  CSXParserHandler::~CSXParserHandler(){
    m_elemStack.clear();
  }

  Item
  CSXParserHandler::getAtomicItem(opencsx::AtomicValue const& v){
    //cout << "Item Type: ";
    switch(v.m_type){
      case opencsx::DT_BOOLEAN:
        cout << "boolean";
        return m_itemFactory->createBoolean(v.m_value.f_bool);
      case opencsx::DT_BYTE:
        cout << "byte";
        return m_itemFactory->createByte(v.m_value.f_char);
      case opencsx::DT_INT:
        //cout << "integer";
        return m_itemFactory->createInt(v.m_value.f_int);
        break;
      case opencsx::DT_LONG:
        cout << "long";
        return m_itemFactory->createLong(v.m_value.f_long);
      case opencsx::DT_FLOAT:
        cout << "float";
        return m_itemFactory->createFloat(v.m_value.f_float);
      case opencsx::DT_DOUBLE:
        cout << "double";
        return m_itemFactory->createDouble(v.m_value.f_double);
      case opencsx::DT_STRING:
        return m_itemFactory->createString(v.m_string);
      default:
        assert(false);
    }
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
