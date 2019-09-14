// test_multipart.cpp

#include <ulib/file.h>
#include <ulib/mime/multipart.h>

int U_EXPORT main(int argc, char* argv[])
{
   U_ULIB_INIT(argv);

   U_TRACE(5,"main(%d)",argc)

   U_line_terminator_len = 1;

   UMimeMultipartMsg msg;
   UString dati, filename;

   while (cin >> filename)
      {
      dati = UFile::contentOf(filename);

      msg.add(UMimeMultipartMsg::section(U_STRING_TO_PARAM(dati), 0, 0, UMimeMultipartMsg::AUTO, "iso-8859-1", filename.data()));
      }

   msg.add(UMimeMultipartMsg::section(U_CONSTANT_TO_PARAM("File di testo\n"),
                                      U_CONSTANT_TO_PARAM("text/plain"),
                                      UMimeMultipartMsg::BIT7,
                                      "iso-8859-1", "",
                                      U_CONSTANT_TO_PARAM("Content-Disposition: inline")));

   cout << msg;
}
