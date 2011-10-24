#include <zorba/item_factory.h>
#include <zorba/singleton_item_sequence.h>
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

    Zorba* lZorba = Zorba::getInstance(0);
    XQuery_t lQuery = lZorba->compileQuery("2+1");
    
    
    return ItemSequence_t(new SingletonItemSequence(lZorba->getItemFactory()->createString("Hi")));
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
    return ItemSequence_t(new SingletonItemSequence(Zorba::getInstance(0)->getItemFactory()->createBase64Binary(base64Result.c_str(),base64Result.length())));
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
