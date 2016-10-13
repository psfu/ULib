// ============================================================================
//
// = LIBRARY
//    ULib - c++ library
//
// = FILENAME
//    tokenizer.h
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_TOKENIZER_H
#define ULIB_TOKENIZER_H 1

#include <ulib/string.h>

class UQueryParser;

template <class T> class UVector;

class U_EXPORT UTokenizer {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   UTokenizer(const char* d = 0)
      {
      U_TRACE_REGISTER_OBJECT(0, UTokenizer, "%S", d)

      s = end = 0;
      delim   = d;
      }

   UTokenizer(const UString& data, const char* d = 0) : str(data)
      {
      U_TRACE_REGISTER_OBJECT(0, UTokenizer, "%V,%S", data.rep, d)

      s     = data.data();
      end   = data.pend();
      delim = d;
      }

   ~UTokenizer()
      {
      U_TRACE_UNREGISTER_OBJECT(0, UTokenizer)
      }

   bool atEnd()
      {
      U_TRACE_NO_PARAM(0, "UTokenizer::atEnd()")

      if (s >= end) U_RETURN(true);

      U_RETURN(false);
      }

   void setDelimiter(const char* sep)
      {
      U_TRACE(0, "UTokenizer::setDelimiter(%S)", sep)

      delim = sep;
      }

   void setData(const UString& data);

   void skipSpaces()
      {
      U_TRACE_NO_PARAM(0, "UTokenizer::skipSpaces()")

      while (s < end && u__isspace(*s)) ++s;
      }

   void setPointer(const char* ptr)
      {
      U_TRACE(0, "UTokenizer::setPointer(%S)", ptr)

      U_INTERNAL_ASSERT(ptr <= end)

      s = ptr;
      }

   const char* getPointer() const
      {
      U_TRACE_NO_PARAM(0, "UTokenizer::getPointer()")

      U_RETURN(s);
      }

   const char* getEnd() const
      {
      U_TRACE_NO_PARAM(0, "UTokenizer::getEnd()")

      U_RETURN(end);
      }

   void setDistance(uint32_t pos)
      {
      U_TRACE(0, "UTokenizer::setDistance(%u)", pos)

      s = str.c_pointer(pos);
      }

   uint32_t getDistance() const __pure
      {
      U_TRACE(0, "UTokenizer::getDistance()")

      uint32_t pos = (s < end ? str.distance(s) : str.size());

      U_RETURN(pos);
      }

   // get current char

   char current()
      {
      U_TRACE_NO_PARAM(0, "UTokenizer::current()")

      if (s >= end) U_RETURN('\0');

      U_RETURN(*s);
      }

   // go prev char

   void back()
      {
      U_TRACE_NO_PARAM(0, "UTokenizer::back()")

      U_INTERNAL_ASSERT_MAJOR(s, str.data())

      --s;
      }

   // get next char

   char next()
      {
      U_TRACE_NO_PARAM(0, "UTokenizer::next()")

      if (s >= end) U_RETURN('\0');

      U_RETURN(*s++);
      }

   UTokenizer& operator++() { ++s; return *this; } // ++A

   // get next token

   bool   next(UString& tok, char c);
   bool extend(UString& tok, char c); // extend the actual token to the next char 'c'... (see PEC_report.cpp)

   bool   next(UString& tok, bPFi func);
   bool   next(UString& tok, bool* bgroup);

   // EXT

   UString substr() const
      {
      U_TRACE_NO_PARAM(0, "UTokenizer::substr()")

      U_INTERNAL_ASSERT(s <= end)

      return (s < end ? str.substr(str.distance(s)) : UString::getStringNull());
      }

   UString substr(const char* start) const
      {
      U_TRACE(0, "UTokenizer::substr(%p)", start)

      U_INTERNAL_ASSERT(start < end)

      return str.substr(start, s - start);
      }

   void skipNumber()
      {
      U_TRACE_NO_PARAM(0, "UTokenizer::skipNumber()")

      for (; s < end; ++s)
         {
         char c = *s;

         if (u__isnumberchar(c) == false &&
             u__toupper(c) != 'E') // scientific notation (Ex: 1.45e-10)
            {
            break;
            }
         }
      }

   int  getTypeNumber();
   int  getTokenId(UString* ptoken);
   bool tokenSeen(const UString* x);
   bool skipToken(const char* token, uint32_t sz);

   UString getTokenQueryParser();

   static const char* group;
   static bool group_skip, avoid_punctuation;
   static uint32_t group_len, group_len_div_2;

   static void setGroup(const char* grp, uint32_t grp_len = 0, bool bskip = false)
      {
      U_TRACE(0, "UTokenizer::setGroup(%S,%u,%b)", grp, grp_len, bskip)

      group           = grp;
      group_skip      = bskip;
      group_len       = grp_len;
      group_len_div_2 = grp_len / 2;
      }

   static void setSkipTagXML(bool flag)
      {
      U_TRACE(0, "UTokenizer::setSkipTagXML(%b)", flag)

      if (flag) setGroup(U_CONSTANT_TO_PARAM("<>"), true);
      else      setGroup(0,                      0, false);
      }

   static void setAvoidPunctuation(bool flag)
      {
      U_TRACE(0, "UTokenizer::setAvoidPunctuation(%b)", flag)

      avoid_punctuation = flag;
      }

#if defined(U_STDCPP_ENABLE) && defined(DEBUG)
   const char* dump(bool reset) const;
#endif

protected:
   const char* s;
   const char* end;
   const char* delim;
   UString str;

private:
   U_DISALLOW_COPY_AND_ASSIGN(UTokenizer)

   friend class UQueryParser;
};

#endif
