#include <zorba/item_factory.h>
#include <zorba/singleton_item_sequence.h>
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

  CSXModule::~CSXModule()
  {
    if(!theParseFunction)
      delete theParseFunction;
    if(!theSerializeFunction)
      delete theSerializeFunction;
  }
  
  /*******************************************************************************************
  *******************************************************************************************/
  zorba::ItemSequence_t
    ParseFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    CSXParserHandler *parserHandler;
    opencsx::CSXSAX2StdParser *parser;
    parserHandler = new CSXParserHandler();
    parser = opencsx::CSXSAX2StdParser::create(opencsx::CSXStdVocabulary::create());
    parser->setContentHandler(parserHandler);
    vector<Item> v;

    try {
      for(int i=0; i<aArgs.size(); i++){
        Iterator_t iter = aArgs[i]->getIterator();
        iter->open();
        Item input;
        if(iter->next(input)){
          string iString = zorba::encoding::Base64::decode(input.getStringValue()).str();
          stringstream ss(iString);
          parser->parse(ss);
        }
      }
    } catch(ZorbaException ze){
      cerr << ze << endl;
    }

    v = parserHandler->getVectorItem();
    return (v.size() > 0)?ItemSequence_t(new SingletonItemSequence(v[0])):NULL;
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

  void transverse(Iterator_t iter, opencsx::CSXHandler* handler){
    // given a iterator transverse the item()* tree
    Item name, item, attr, aName;
    Iterator_t children, attrs;
    // here comes the magic :)
    iter->open();
    while(iter->next(item)){
      if(item.isNode()){
        int kind = item.getNodeKind();
        cout << "node type: " << getKindAsString(kind) << endl;
        if(kind != zorba::store::StoreConsts::textNode){
          cout << "<StartElement>" << endl;
          item.getNodeName(name);
          cout << "name: " << name.getNamespace() << "::" << name.getPrefix() << "::" << name.getLocalName() << endl;
          handler->startElement(name.getNamespace().str(), name.getLocalName().str(),
                                name.getPrefix().str());

          // go thru attributes
          attrs = item.getAttributes();
          if(!attrs.isNull()){
            attrs->open();
            while(attrs->next(attr)){
              kind = attr.getNodeKind();
              cout << "attr type: " << getKindAsString(kind) << endl;
              attr.getNodeName(aName);
              cout << "attr name: " << aName.getNamespace() << "::" << aName.getPrefix() << "::" << aName.getLocalName() << endl;
              cout << "attr text: " << attr.getStringValue() << endl;
              handler->attribute(aName.getNamespace().str(), aName.getLocalName().str(),
                                 aName.getPrefix().str(), attr.getStringValue().str());
            }
          }

          children = item.getChildren();
          if(!children.isNull()){
            // there is at least one child
            transverse(children, handler);
          }

          cout << "<EndElement>" << endl;
          handler->endElement(name.getNamespace().str(), name.getLocalName().str(),
                              name.getPrefix().str());
        } else {
          cout << "<Characters>" << endl;
          handler->characters(item.getStringValue().str());
          children = NULL;
          cout << "text content: " << item.getStringValue() << endl;
        }
      }
    }
    // here ends the magic :(
    iter->close();
  }

  zorba::ItemSequence_t
    SerializeFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    void *lStore = zorba::StoreManager::getStore();
    Zorba* lZorba = Zorba::getInstance(lStore);
    stringstream theOutputStream;
    opencsx::CSXHandler* csxHandler = theCsxManager->createSerializer(theOutputStream);

    try {
      cout << "<StartDocument>" << endl;
      csxHandler->startDocument();
      for(int i=0; i<aArgs.size(); i++){
        Iterator_t iter = aArgs[i]->getIterator();
        transverse(iter, csxHandler);
      }
      cout << "<EndDocument>" << endl;
      csxHandler->endDocument();
    } catch(ZorbaException ze){
      cerr << ze << endl;
    }

    cout << ">>>>Returning (no Base64): " << theOutputStream << endl;
    string base64Result = zorba::encoding::Base64::encode(theOutputStream.str()).str();
    return ItemSequence_t(
      new SingletonItemSequence(Zorba::getInstance(0)->getItemFactory()->createBase64Binary(base64Result.c_str(),base64Result.length()))
    );
  }

  /* CSXHandler implementation here */
  CSXParserHandler::CSXParserHandler(void){
    m_defaultType = Zorba::getInstance(0)->getItemFactory()->createQName(zorba::String("http://www.w3.org/2001/XMLSchema"),zorba::String("untyped"));
    m_defaultAttrType = Zorba::getInstance(0)->getItemFactory()->createQName(zorba::String("http://www.w3.org/2001/XMLSchema"),zorba::String("AnyAtomicType"));
  }

  void CSXParserHandler::startDocument(){
  }

  void CSXParserHandler::endDocument(){
    m_itemStack.clear();
    m_nsVector.clear();
  }

  void CSXParserHandler::startElement(const string uri, const string localname, const string prefix, const opencsx::CSXStdAttributes* attrs){
    Item m_possibleParent;
    zorba::String zUri = zorba::String(uri);
    zorba::String zLocalName = zorba::String(localname);
    zorba::String zPrefix = zorba::String(prefix);
    Zorba* z = Zorba::getInstance(0);
    Item nodeName = z->getItemFactory()->createQName(zUri, zPrefix, zLocalName);
    Item attrNodeValue;
    m_possibleParent = Zorba::getInstance(0)->getItemFactory()->createElementNode(m_parent, nodeName, m_defaultType, false, false, m_nsVector);
    if(attrs!= NULL && attrs->getLength() > 0){
      for(int i=0; i<attrs->getLength(); i++){
        zorba::String zAttrName = attrs->getLocalName(i);
        zUri = attrs->getURI(i);
        zPrefix = attrs->getQName(i);
        nodeName = z->getItemFactory()->createQName(zUri, zPrefix, zLocalName);
        attrNodeValue = z->getItemFactory()->createString(zorba::String(attrs->getValue(i)));
        z->getItemFactory()->createAttributeNode(m_possibleParent, nodeName, m_defaultAttrType, attrNodeValue);
      }
    }
    if(m_parent.isNull() || prefix == m_parent.getPrefix().str()+m_parent.getLocalName().str()){
      // m_ptrParent is the actual father so replace m_ptrParent for this node 
      // so if there is any childen then they will need to have a pointer to this
      // node for creation
      m_parent = m_possibleParent;
    }
  }

  void CSXParserHandler::endElement(const string uri, const string localname, const string qname){
    if(m_parent.isNull()){
      // the current parent is the root element so this node should be added
      // to m_itemVector
      m_itemStack.push_back(m_currentItem);
    }
    m_parent = (m_parent.isNull())?m_parent.getParent():m_emptyItem;
    m_nsVector.clear();
  }

  void CSXParserHandler::characters(const string chars){
    // create text node
    m_currentItem = Zorba::getInstance(0)->getItemFactory()->createTextNode(m_parent, chars);
  }

  void CSXParserHandler::startPrefixMapping(const string prefix, const string uri){
    m_nsVector.push_back(pair<zorba::String,zorba::String>(zorba::String(prefix),zorba::String(uri)));
  }

  void CSXParserHandler::endPrefixMapping(const string prefix){
    // No need to implement ... I think
  }

  void CSXParserHandler::startElement
  (const string uri, const string localname, const string prefix){
    // No need to implement
  }

  void CSXParserHandler::attribute(const string uri, const string localname, const string qname, const string value){
    // No need to implement
  }

  void CSXParserHandler::declareNamespace(const string prefix, const string uri){
    // No need to implement
  }

  void CSXParserHandler::processingInstruction(const string target, const string data){
    // No need to implement
  }

  void CSXParserHandler::comment(const string chars){
    // No need to implement
  }

  vector<Item> CSXParserHandler::getVectorItem(){
    return m_itemStack;
  }

  CSXParserHandler::~CSXParserHandler(){
    m_itemStack.clear();
    m_nsVector.clear();
  }
 
}/*namespace csx*/ }/*namespace zorba*/

#ifdef WIN32
#  define DLL_EXPORT __declspec(dllexport)
#else
#  define DLL_EXPORT __attribute__ ((visibility("default")))
#endif

extern "C" DLL_EXPORT zorba::ExternalModule* createModule() {
  return new zorba::csx::CSXModule();
}
