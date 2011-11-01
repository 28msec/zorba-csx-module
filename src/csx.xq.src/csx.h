#ifndef __COM_ZORBA_WWW_MODULES_CSX_H__
#define __COM_ZORBA_WWW_MODULES_CSX_H__

#include <zorba/zorba.h>
#include <zorba/external_module.h>
#include <zorba/store_manager.h>
#include <zorba/function.h>
#include <opencsx/csxstdmanager.h>
#include <opencsx/csxhandler.h>
#include <opencsx/csxstdattributes.h>
#include <opencsx/stdvocab.h>
#include <opencsx/csxsax2stdparser.h>
#include <vector>

namespace zorba { namespace csx {

  class CSXModule : public ExternalModule {
		private:

      const static String CSX_MODULE_NAMESPACE;

      ExternalFunction* theParseFunction;
      ExternalFunction* theSerializeFunction;

    public:

      inline CSXModule():theParseFunction(0), theSerializeFunction(0){}

      virtual ~CSXModule();

      virtual zorba::String 
        getURI() const {return CSX_MODULE_NAMESPACE;}

      virtual zorba::ExternalFunction* 
        getExternalFunction(const String& localName);

      virtual void destroy();
  };

  class ParseFunction : public ContextualExternalFunction{
    public:
      ParseFunction(const CSXModule* aModule) : theModule(aModule) {}

      virtual ~ParseFunction(){}

      virtual zorba::String
        getLocalName() const { return "parse"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;

      virtual String getURI() const {
        return theModule->getURI();
      }

    protected:
      const CSXModule *theModule;
  };

  class SerializeFunction : public ContextualExternalFunction{
  private:
    opencsx::CSXStdManager* theCsxManager;
  public:
    SerializeFunction(const CSXModule* aModule) : theModule(aModule) {
      theCsxManager = opencsx::CSXStdManager::create();
    }

    virtual ~SerializeFunction(){
      delete theCsxManager;
    }

    virtual zorba::String
      getLocalName() const { return "serialize"; }

    virtual zorba::ItemSequence_t
      evaluate(const Arguments_t&,
                const zorba::StaticContext*,
                const zorba::DynamicContext*) const;

    virtual String getURI() const {
      return theModule->getURI();
    }

  protected:
    const CSXModule* theModule;
  };

  class CSXParserHandler : public opencsx::CSXHandler {
  public:
    void startDocument();
    void endDocument();
    void startElement(const string uri, const string localname, const string prefix);
    void startElement(const string uri, const string localname, const string prefix, const opencsx::CSXStdAttributes* attrs);
    void endElement(const string uri, const string localname, const string qname);
    void attribute(const string uri, const string localname, const string qname, const string value);
    void characters(const string chars);
    void declareNamespace(const string prefix, const string uri);
    void processingInstruction(const string target, const string data);
    void comment(const string chars);
    void startPrefixMapping(const string prefix, const string uri);
    void endPrefixMapping(const string prefix);
    vector<Item> getVectorItem();
    CSXParserHandler(void);
    virtual ~CSXParserHandler(void);
  private:
    vector<Item> m_itemStack;
    vector<pair<zorba::String,zorba::String>> m_nsVector;
    Item m_currentItem;
    Item m_defaultType;
    Item m_defaultAttrType;
    Item m_parent;
    Item m_emptyItem; // needed to have a false NULL item
  };

}/*csx namespace*/}/*zorba namespace*/


#endif //_COM_ZORBA_WWW_MODULES_CSX_H_
