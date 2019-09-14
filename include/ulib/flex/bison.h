// ============================================================================
//
// = LIBRARY
//    ULib - c++ library
//
// = FILENAME
//    bison.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef U_BISON_H
#define U_BISON_H

#include <ulib/flex/flexer.h>

/**
 * @class UBison
 *
 * Implementazione of Bison for ULib
 */

// extern int yydebug;
   extern int yyparse(void*);

class U_EXPORT UBison : public UFlexer {
public:

   UBison()
      {
      U_TRACE_CTOR(0, UBison, "")
      }

   UBison(const UString& data_) : UFlexer(data_)
      {
      U_TRACE_CTOR(0, UBison, "%V", data_.rep)
      }

   ~UBison()
      {
      U_TRACE_DTOR(0, UBison)
      }

   bool parse(void* obj = U_NULLPTR)
      {
      U_TRACE(0, "UBison::parse(%p)", obj)

      U_INTERNAL_ASSERT(data)

   // yydebug = 1;

      if (obj == U_NULLPTR) obj = this;

      if (yyparse(obj) == 0)
         {
         U_INTERNAL_DUMP("UFlexer::parsed_chars = %d, UFlexer::data.size() = %u", UFlexer::parsed_chars, UFlexer::data.size())

         U_RETURN(true);
         }

      U_RETURN(false);
      }

   bool parse(const UString& _data, void* obj = U_NULLPTR)
      {
      U_TRACE(0, "UBison::parse(%V,%p)", _data.rep, obj)

      setData(_data);

      if (parse(obj)) U_RETURN(true);

      U_RETURN(false);
      }

   // DEBUG

#if defined(U_STDCPP_ENABLE) && defined(DEBUG)
   const char* dump(bool reset) const;
#endif

private:
   U_DISALLOW_COPY_AND_ASSIGN(UBison)
};

#endif
