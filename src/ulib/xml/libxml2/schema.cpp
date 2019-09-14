// ============================================================================
//
// = LIBRARY
//    ULib - c++ library
//
// = FILENAME
//    schema.cpp - wrapping of libxml2
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/xml/libxml2/schema.h>

// Do nothing function
// static void foo(void* ctx, const char* msg, ...) {}

UXML2Schema::UXML2Schema(const UString& xmldoc)
{
   U_TRACE_CTOR(0, UXML2Schema, "%V", xmldoc.rep)

   xmlSchemaParserCtxtPtr context = (xmlSchemaParserCtxtPtr) U_SYSCALL(xmlSchemaNewMemParserCtxt, "%S,%u", U_STRING_TO_PARAM(xmldoc));

   U_SYSCALL_VOID(xmlSchemaSetParserErrors, "%p,%p,%p,%p", context, (xmlSchemaValidityErrorFunc)U_NULLPTR, (xmlSchemaValidityWarningFunc)U_NULLPTR, U_NULLPTR);

// xmlGenericError        = foo;
   xmlGenericErrorContext = U_NULLPTR;

   ctxt  = U_NULLPTR;
   impl_ = (xmlSchemaPtr) U_SYSCALL(xmlSchemaParse, "%p", context);

   U_SYSCALL_VOID(xmlSchemaFreeParserCtxt, "%p", context);

   if (impl_)
      {
      ctxt = (xmlSchemaValidCtxtPtr) U_SYSCALL(xmlSchemaNewValidCtxt, "%p", impl_);

      U_SYSCALL_VOID(xmlSchemaSetValidErrors, "%p,%p,%p,%p", ctxt, (xmlSchemaValidityErrorFunc)U_NULLPTR, (xmlSchemaValidityWarningFunc)U_NULLPTR, U_NULLPTR);
      }
}

UXML2Schema::~UXML2Schema()
{
   U_TRACE_DTOR(0, UXML2Schema)

             U_SYSCALL_VOID(xmlSchemaFree,          "%p", impl_);
   if (ctxt) U_SYSCALL_VOID(xmlSchemaFreeValidCtxt, "%p", ctxt);

   U_SYSCALL_VOID_NO_PARAM(xmlSchemaCleanupTypes);
}

void UXML2Schema::writeToFile(const char* filename)
{
   U_TRACE(1, "UXML2Schema::writeToFile(%S)", filename)

   U_INTERNAL_ASSERT_POINTER(impl_)

   FILE* f = fopen(filename, "wb");

   U_SYSCALL_VOID(xmlSchemaDump, "%p,%p", f, impl_);

   fclose(f);
}

bool UXML2Schema::validate(UXML2Document& doc)
{
   U_TRACE(1, "UXML2Schema::validate(%p)", &doc)

   U_INTERNAL_ASSERT_POINTER(impl_)

   bool reuse = (ctxt != U_NULLPTR);

   if (reuse == false)
      {
      ctxt = (xmlSchemaValidCtxtPtr) U_SYSCALL(xmlSchemaNewValidCtxt, "%p", impl_);

      U_SYSCALL_VOID(xmlSchemaSetValidErrors, "%p,%p,%p,%p", ctxt, (xmlSchemaValidityErrorFunc)U_NULLPTR, (xmlSchemaValidityWarningFunc)U_NULLPTR, U_NULLPTR);
      }

   bool result = (U_SYSCALL(xmlSchemaValidateDoc, "%p,%p", ctxt, doc.cobj()) == 0);

   if (reuse == false)
      {
      U_SYSCALL_VOID(xmlSchemaFreeValidCtxt, "%p", ctxt);

      ctxt = U_NULLPTR;
      }

   U_RETURN(result);
}

// DEBUG

#if defined(U_STDCPP_ENABLE) && defined(DEBUG)
const char* UXML2Schema::dump(bool reset) const
{
   *UObjectIO::os << "ctxt  " << (void*)ctxt << '\n'
                  << "impl_ " << (void*)impl_;

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return U_NULLPTR;
}
#endif
