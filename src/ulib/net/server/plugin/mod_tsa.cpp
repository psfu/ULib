// ============================================================================
//
// = LIBRARY
//    ULib - c++ library
//
// = FILENAME
//    mod_tsa.cpp - this is a plugin tsa for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/command.h>
#include <ulib/file_config.h>
#include <ulib/utility/uhttp.h>
#include <ulib/net/server/server.h>
#include <ulib/net/server/plugin/mod_tsa.h>

U_CREAT_FUNC(server_plugin_tsa, UTsaPlugIn)

UCommand* UTsaPlugIn::command;

UTsaPlugIn::UTsaPlugIn()
{
   U_TRACE_CTOR(0, UTsaPlugIn, "")
}

UTsaPlugIn::~UTsaPlugIn()
{
   U_TRACE_DTOR(0, UTsaPlugIn)

   if (command) U_DELETE(command)
}

// Server-wide hooks

int UTsaPlugIn::handlerConfig(UFileConfig& cfg)
{
   U_TRACE(0, "UTsaPlugIn::handlerConfig(%p)", &cfg)

   // -----------------------------------------------
   // Perform registration of userver method
   // -----------------------------------------------
   // COMMAND                      command to execute
   // ENVIRONMENT  environment for command to execute
   // -----------------------------------------------

   command = UServer_Base::loadConfigCommand();

   U_RETURN(U_PLUGIN_HANDLER_PROCESSED);
}

int UTsaPlugIn::handlerInit()
{
   U_TRACE_NO_PARAM(0, "UTsaPlugIn::handlerInit()")

   if (command)
      {
      // NB: tsa is NOT a static page, so to avoid stat() syscall we use alias mechanism...

#  ifndef U_ALIAS
      U_SRV_LOG("WARNING: Sorry, I can't enable TSA plugin because alias URI support is missing, please recompile ULib");
#  else
      if (UHTTP::valias == U_NULLPTR) U_NEW(UVector<UString>, UHTTP::valias, UVector<UString>(2U))

      UHTTP::valias->push_back(*UString::str_tsa);
      UHTTP::valias->push_back(*UString::str_nostat);

      U_RETURN(U_PLUGIN_HANDLER_OK);
#  endif
      }

   U_RETURN(U_PLUGIN_HANDLER_ERROR);
}

// Connection-wide hooks

int UTsaPlugIn::handlerRequest()
{
   U_TRACE_NO_PARAM(0, "UTsaPlugIn::handlerRequest()")

   if (UHTTP::isTSARequest())
      {
      // NB: process the HTTP tsa request with fork....

      if (UServer_Base::startParallelization()) U_RETURN(U_PLUGIN_HANDLER_PROCESSED); // parent 

      UString body;

      if (command->execute(UHTTP::body, &body) == false) UHTTP::setInternalError();
      else                                               UHTTP::setResponse(*UString::str_ctype_tsa, &body);

      U_SRV_LOG_CMD_MSG_ERR(*command, true);

      U_RETURN(U_PLUGIN_HANDLER_PROCESSED);
      }

   U_RETURN(U_PLUGIN_HANDLER_OK);
}

// DEBUG

#if defined(U_STDCPP_ENABLE) && defined(DEBUG)
const char* UTsaPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "command (UCommand " << (void*)command << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return U_NULLPTR;
}
#endif
