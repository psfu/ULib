// ============================================================================
//
// = LIBRARY
//    ULib - c++ library
//
// = FILENAME
//    mod_rpc.cpp - this is a plugin rpc for userver
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#include <ulib/net/rpc/rpc.h>
#include <ulib/net/server/server.h>
#include <ulib/net/rpc/rpc_object.h>
#include <ulib/net/rpc/rpc_parser.h>
#include <ulib/net/server/plugin/mod_rpc.h>

U_CREAT_FUNC(server_plugin_rpc, URpcPlugIn)

bool        URpcPlugIn::is_rpc_msg;
URPCParser* URpcPlugIn::rpc_parser;

URpcPlugIn::~URpcPlugIn()
{
   U_TRACE_DTOR(0, URpcPlugIn)

   if (rpc_parser)
      {
      U_DELETE(rpc_parser)
      U_DELETE(URPCMethod::encoder)
      U_DELETE(URPCObject::dispatcher)
      }
}

// Server-wide hooks

int URpcPlugIn::handlerInit()
{
   U_TRACE_NO_PARAM(0, "URpcPlugIn::handlerInit()")

   // Perform registration of server RPC method

   if (UServer_Base::pcfg &&
       UServer_Base::pcfg->searchForObjectStream(U_CONSTANT_TO_PARAM("rpc")))
      {
      UServer_Base::pcfg->table.clear();

      U_NEW(URPCParser, rpc_parser, URPCParser);

      URPCObject::loadGenericMethod(UServer_Base::pcfg);
      }

   U_RETURN(U_PLUGIN_HANDLER_PROCESSED);
}

// Connection-wide hooks

int URpcPlugIn::handlerREAD()
{
   U_TRACE_NO_PARAM(0, "URpcPlugIn::handlerREAD()")

   if (rpc_parser)
      {
      is_rpc_msg = URPC::readRequest(UServer_Base::csocket); // NB: URPC::resetInfo() it is already called by clearData()...
      }

   U_RETURN(U_PLUGIN_HANDLER_OK);
}

int URpcPlugIn::handlerRequest()
{
   U_TRACE_NO_PARAM(0, "URpcPlugIn::handlerRequest()")

   if (is_rpc_msg)
      {
      // process the RPC request

      U_INTERNAL_ASSERT_POINTER(rpc_parser)
      U_INTERNAL_ASSERT(*UClientImage_Base::request)

      bool bSendingFault;
      UString method = UClientImage_Base::request->substr(0U, U_TOKEN_NM);

      *UClientImage_Base::wbuffer = rpc_parser->processMessage(method, *URPCObject::dispatcher, bSendingFault);

      U_SRV_LOG_WITH_ADDR("method %V process %s for", method.rep, (bSendingFault ? "failed" : "passed"));

#  ifndef U_LOG_DISABLE
      if (UServer_Base::isLog()) (void) UClientImage_Base::request_uri->assign(method);
#  endif

      UClientImage_Base::bnoheader = true;

      UClientImage_Base::setRequestProcessed();

      U_RETURN(U_PLUGIN_HANDLER_PROCESSED);
      }

   U_RETURN(U_PLUGIN_HANDLER_OK);
}

// DEBUG

#if defined(U_STDCPP_ENABLE) && defined(DEBUG)
const char* URpcPlugIn::dump(bool reset) const
{
   *UObjectIO::os << "is_rpc_msg             " << is_rpc_msg        << '\n'
                  << "rpc_parser (URPCParser " << (void*)rpc_parser << ')';

   if (reset)
      {
      UObjectIO::output();

      return UObjectIO::buffer_output;
      }

   return U_NULLPTR;
}
#endif
