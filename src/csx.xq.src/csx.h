#ifndef __COM_ZORBA_WWW_MODULES_CSX_H__
#define __COM_ZORBA_WWW_MODULES_CSX_H__

#include <zorba/zorba.h>
#include <zorba/external_module.h>
#include <zorba/store_manager.h>
#include <zorba/function.h>
#include <opencsx/csxhandler.h>
#include <opencsx/csxprocessor.h>
#include <opencsx/stdvocab.h>
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

      void loadVocab(opencsx::CSXProcessor* aProc, Iterator_t aUriIter) const;
  };

  class ParseFunction : public ContextualExternalFunction{
    public:
      ParseFunction(const CSXModule* aModule) : theModule(aModule) {
        theProcessor = opencsx::CSXProcessor::create();
      }

      virtual ~ParseFunction() {
        delete theProcessor;
      }

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
    private:
      opencsx::CSXProcessor* theProcessor;
  };

  class SerializeFunction : public ContextualExternalFunction{
    public:
      SerializeFunction(const CSXModule* aModule) : theModule(aModule) {
        theProcessor = opencsx::CSXProcessor::create();
      }

      virtual ~SerializeFunction(){
        delete theProcessor;
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
    private:
      opencsx::CSXProcessor* theProcessor;
  };

  class CSXParserHandler : public opencsx::CSXHandler {
    public:
      void startDocument();
      void endDocument();
      void startElement(const string& uri, const string& localname,
                        const string& prefix, const opencsx::CSXHandler::NsBindings* bindings);
      void endElement(const string &uri, const string &localname, const string &prefix);
      void attribute(const string &uri, const string &localname, const string &prefix,
                     opencsx::AtomicValue const& value);
      void atomicValue(const opencsx::AtomicValue &value);
      void processingInstruction(const string &target, const string &data);
      void comment(const string &chars);
      CSXParserHandler(vector<Item>& aItems);
      virtual ~CSXParserHandler();
    private:
      Item getAtomicItem(opencsx::AtomicValue const& v);

      ItemFactory* m_itemFactory;

      // Stack of constructed elements
      vector<Item> m_elemStack;

      // Output of parsing CSX
      vector<Item>& m_result;

      // Collects AtomicValues for passing to assignElementTypedValue()
      vector<Item> m_atomics;

      // Defaults for untyped values
      Item m_defaultType;
      Item m_defaultAttrType;
  };

}/*csx namespace*/}/*zorba namespace*/


#endif //_COM_ZORBA_WWW_MODULES_CSX_H_
