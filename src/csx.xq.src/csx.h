#ifndef __COM_ZORBA_WWW_MODULES_CSX_H__
#define __COM_ZORBA_WWW_MODULES_CSX_H__

#include <zorba/zorba.h>
#include <zorba/external_module.h>
#include <zorba/store_manager.h>
#include <zorba/function.h>
#include <opencsx/csxstdmanager.h>
#include <opencsx/csxhandler.h>

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
      const CSXModule* theModule;

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

}/*csx namespace*/}/*zorba namespace*/


#endif //_COM_ZORBA_WWW_MODULES_CSX_H_
