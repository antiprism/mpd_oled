/*
 *  Http put/get mini lib
 *  written by L. Demailly
 *  (c) 1998 Laurent Demailly - http://www.demailly.com/~dl/
 *  (c) 1996 Observatoire de Paris - Meudon - France
 *  see LICENSE for terms, conditions and DISCLAIMER OF ALL WARRANTIES
 *  LICENSE: Artistic Licence 2.0 https://opensource.org/licenses/Artistic-2.0
 *
 * $Id: http_lib.h,v 1.4 1998/09/23 06:14:15 dl Exp $
 *
 * Modified by Adrian Rossiter <adrian@antiprism.com> 11/11/2019:
 *   Compile with C++. Wrap in class to remove global variables.
 */

#include <string>


class HttpRequest {
private:
  enum querymode{
    CLOSE,    /* Close the socket after the query (for put) */
    KEEP_OPEN /* Keep it open */
  };

  /* return type */
  enum http_retcode {

    /* Client side errors */
    ERRHOST = -1,  /* No such host */
    ERRSOCK = -2,  /* Can't create socket */
    ERRCONN = -3,  /* Can't connect to host */
    ERRWRHD = -4,  /* Write error on socket while writing header */
    ERRWRDT = -5,  /* Write error on socket while writing data */
    ERRRDHD = -6,  /* Read error on socket while reading result */
    ERRPAHD = -7,  /* Invalid answer from data server */
    ERRNULL = -8,  /* Null data pointer */
    ERRNOLG = -9,  /* No/Bad length in header */
    ERRMEM = -10,  /* Can't allocate memory */
    ERRRDDT = -11, /* Read error while reading data */
    ERRURLH = -12, /* Invalid url - must start with 'http://' */
    ERRURLP = -13, /* Invalid port in url */

    /* Return code by the server */
    ERR400 = 400, /* Invalid query */
    ERR403 = 403, /* Forbidden */
    ERR408 = 408, /* Request timeout */
    ERR500 = 500, /* Server error */
    ERR501 = 501, /* Not implemented */
    ERR503 = 503, /* Service overloaded */

    /* Succesful results */
    OK0 = 0,     /* successfull parse */
    OK201 = 201, /* Ressource succesfully created */
    OK200 = 200  /* Ressource succesfully read */
  };

  std::string http_server;
  int http_port = 0;
  std::string http_proxy_server;
  int http_proxy_port = 0;
  std::string fname;

  http_retcode http_query(const char *command, const char *url,
                          const char *additional_header, querymode mode,
                          const char *data, int length, int *pfd);

public:
  http_retcode http_put(const char *filename, char *data, int length, int overwrite,
                        char *type);
  http_retcode http_get(const char *filename, char **pdata, int *plength,
                        char *typebuf);
  http_retcode http_parse_url(char *url, char **pfilename);
  http_retcode http_delete(const char *filename);
  http_retcode http_head(const char *filename, int *plength, char *typebuf);
  http_retcode set_proxy(char *proxy);

  http_retcode set_url(const std::string &url);
  std::string get();

};
