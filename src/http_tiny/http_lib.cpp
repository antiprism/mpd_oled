/*
 *  Http put/get mini lib
 *  written by L. Demailly
 *  (c) 1998 Laurent Demailly - http://www.demailly.com/~dl/
 *  (c) 1996 Observatoire de Paris - Meudon - France
 *  see LICENSE for terms, conditions and DISCLAIMER OF ALL WARRANTIES
 *  LICENSE: Artistic Licence 2.0 https://opensource.org/licenses/Artistic-2.0
 *
 * $Id: http_lib.c,v 3.5 1998/09/23 06:19:15 dl Exp $
 *
 * Description : Use http protocol, connects to server to echange data
 *
 * $Log: http_lib.c,v $
 * Revision 3.5  1998/09/23 06:19:15  dl
 * portability and http 1.x (1.1 and later) compatibility
 *
 * Revision 3.4  1998/09/23 05:44:27  dl
 * added support for HTTP/1.x answers
 *
 * Revision 3.3  1996/04/25 19:07:22  dl
 * using intermediate variable for htons (port) so it does not yell
 * on freebsd  (thx pp for report)
 *
 * Revision 3.2  1996/04/24  13:56:08  dl
 * added proxy support through http_proxy_server & http_proxy_port
 * some httpd *needs* cr+lf so provide them
 * simplification + cleanup
 *
 * Revision 3.1  1996/04/18  13:53:13  dl
 * http-tiny release 1.0
 *
 * Modified by Adrian Rossiter <adrian@antiprism.com> 11/11/2019:
 *   Compile with C++. Wrap in class to remove global variables.
 */

//static char *rcsid = "$Id: http_lib.c,v 3.5 1998/09/23 06:19:15 dl Exp $";

#undef VERBOSE

/* http_lib - Http data exchanges mini library.
 */

/* unix */
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <stdio.h>

#include "http_lib.h"

#define SERVER_DEFAULT "adonis"

/* beware that filename+type+rest of header must not exceed MAXBUF */
/* so we limit filename to 256 and type to 64 chars in put & get */
#define MAXBUF 512

/* pointer to a mallocated string containing server name or NULL */
char *http_server = NULL;
/* server port number */
int http_port = 5757;
/* pointer to proxy server name or NULL */
char *http_proxy_server = NULL;
/* proxy server port number or 0 */
int http_proxy_port = 0;
/* user agent id string */
static char http_user_agent[] = "adlib/3 ($Date: 1998/09/23 06:19:15 $)";

namespace {

/*
 * read a line from file descriptor
 * returns the number of bytes read. negative if a read error occured
 * before the end of line or the max.
 * cariage returns (CR) are ignored.
 */
int http_read_line(int fd,       /* file descriptor to read from */
                   char *buffer, /* placeholder for data */
                   int max       /* max number of bytes to read */
)
{ /* not efficient on long lines (multiple unbuffered 1 char reads) */
  int n = 0;
  while (n < max) {
    if (read(fd, buffer, 1) != 1) {
      n = -n;
      break;
    }
    n++;
    if (*buffer == '\015')
      continue; /* ignore CR */
    if (*buffer == '\012')
      break; /* LF is the separator */
    buffer++;
  }
  *buffer = 0;
  return n;
}

/*
 * read data from file descriptor
 * retries reading until the number of bytes requested is read.
 * returns the number of bytes read. negative if a read error (EOF) occured
 * before the requested length.
 */
int http_read_buffer(int fd,       /* file descriptor to read from */
                     char *buffer, /* placeholder for data */
                     int length    /* number of bytes to read */
)
{
  int n, r;
  for (n = 0; n < length; n += r) {
    r = read(fd, buffer, length - n);
    if (r <= 0)
      return -n;
    buffer += r;
  }
  return n;
}

}; // namespace

HttpRequest::http_retcode  HttpRequest::set_proxy(char *proxy)
{
  char *filename = nullptr;
  auto ret = http_parse_url(proxy, &filename);
  free(filename);
  if (ret >= 0) {
    http_proxy_server = http_server;
    http_server.clear();
    http_proxy_port = http_port;
  }
  return ret;
}


/*
 * Pseudo general http query
 *
 * send a command and additional headers to the http server.
 * optionally through the proxy (if http_proxy_server and http_proxy_port are
 * set).
 *
 * Limitations: the url is truncated to first 256 chars and
 * the server name to 128 in case of proxy request.
 */
HttpRequest::http_retcode HttpRequest::http_query(
    const char *command,           /* command to send  */
    const char *url,               /* url / filename queried  */
    const char *additional_header, /* additional header */
    querymode mode,                /* type of query */
    const char *data, /* Data to send after header. If NULL, not data is sent */
    int length,       /* size of data */
    int *pfd /* pointer to variable where to set file descriptor value */
)
{
  int s;
  struct hostent *hp;
  struct sockaddr_in server;
  char header[MAXBUF];
  int hlg;
  http_retcode ret;
  int proxy = (http_proxy_server.size() && http_proxy_port != 0);
  int port = proxy ? http_proxy_port : http_port;

  if (pfd)
    *pfd = -1;

  /* get host info by name :*/
  if ((hp = gethostbyname(proxy ? http_proxy_server.c_str()
                                : (http_server.size() ? http_server.c_str()
                                                      : SERVER_DEFAULT)))) {
    memset((char *)&server, 0, sizeof(server));
    memmove((char *)&server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_family = hp->h_addrtype;
    server.sin_port = (unsigned short)htons(port);
  } else
    return ERRHOST;

  /* create socket */
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return ERRSOCK;
  setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, 0, 0);

  /* connect to server */
  if (connect(s, (const struct sockaddr *)(&server), sizeof(server)) < 0)
    ret = ERRCONN;
  else {
    if (pfd)
      *pfd = s;

    /* create header */
    if (proxy) {
      sprintf(header,
              "%s http://%.128s:%d/%.256s HTTP/1.0\015\012User-Agent: "
              "%s\015\012%s\015\012",
              command, http_server.c_str(), http_port, url, http_user_agent,
              additional_header);
    } else {
      sprintf(header,
              "%s /%.256s HTTP/1.0\015\012User-Agent: %s\015\012%s\015\012",
              command, url, http_user_agent, additional_header);
    }

    hlg = strlen(header);

    /* send header */
    if (write(s, header, hlg) != hlg)
      ret = ERRWRHD;

    /* send data */
    else if (length && data && (write(s, data, length) != length))
      ret = ERRWRDT;

    else {
      /* read result & check */
      ret = static_cast<http_retcode>(http_read_line(s, header, MAXBUF - 1));
#ifdef VERBOSE
      fputs(header, stderr);
      putc('\n', stderr);
#endif
      if (ret <= 0)
        ret = ERRRDHD;
      else if (sscanf(header, "HTTP/1.%*d %03d", (int *)&ret) != 1)
        ret = ERRPAHD;
      else if (mode == KEEP_OPEN)
        return ret;
    }
  }
  /* close socket */
  close(s);
  return ret;
}

/*
 * Put data on the server
 *
 * This function sends data to the http data server.
 * The data will be stored under the ressource name filename.
 * returns a negative error code or a positive code from the server
 *
 * limitations: filename is truncated to first 256 characters
 *              and type to 64.
 */
HttpRequest::http_retcode
HttpRequest::http_put(const char *filename, /* name of the ressource to create */
         char *data,     /* pointer to the data to send   */
         int length,     /* length of the data to send  */
         int overwrite,  /* flag to request to overwrite the ressource if it
                            was already existing */
         char *type      /* type of the data, if NULL default type is used */
)
{
  char header[MAXBUF];
  if (type)
    sprintf(header, "Content-length: %d\015\012Content-type: %.64s\015\012%s",
            length, type, overwrite ? "Control: overwrite=1\015\012" : "");
  else
    sprintf(header, "Content-length: %d\015\012%s", length,
            overwrite ? "Control: overwrite=1\015\012" : "");
  return http_query("PUT", filename, header, CLOSE, data, length, NULL);
}

/*
 * Get data from the server
 *
 * This function gets data from the http data server.
 * The data is read from the ressource named filename.
 * Address of new new allocated memory block is filled in pdata
 * whose length is returned via plength.
 *
 * returns a negative error code or a positive code from the server
 *
 *
 * limitations: filename is truncated to first 256 characters
 */
HttpRequest::http_retcode HttpRequest::http_get(
    const char *filename, /* name of the ressource to read */
    char **pdata,   /* address of a pointer variable which will be set
                       to point toward allocated memory containing read data.*/
    int *plength,   /* address of integer variable which will be set to
                       length of the read data */
    char *typebuf   /* allocated buffer where the read data type is returned.
                     If NULL, the type is not returned */
)
{
  http_retcode ret;

  char header[MAXBUF];
  char *pc;
  int fd;
  int n, length = -1;

  if (!pdata)
    return ERRNULL;
  else
    *pdata = NULL;
  if (plength)
    *plength = 0;
  if (typebuf)
    *typebuf = '\0';

  ret = http_query("GET", filename, "", KEEP_OPEN, NULL, 0, &fd);
  if (ret == 200) {
    while (1) {
      n = http_read_line(fd, header, MAXBUF - 1);
#ifdef VERBOSE
      fputs(header, stderr);
      putc('\n', stderr);
#endif
      if (n <= 0) {
        close(fd);
        return ERRRDHD;
      }
      /* empty line ? (=> end of header) */
      if (n > 0 && (*header) == '\0')
        break;
      /* try to parse some keywords : */
      /* convert to lower case 'till a : is found or end of string */
      for (pc = header; (*pc != ':' && *pc); pc++)
        *pc = tolower(*pc);
      sscanf(header, "content-length: %d", &length);
      if (typebuf)
        sscanf(header, "content-type: %s", typebuf);
    }
    if (length < 0) {
      close(fd);
      return ERRNOLG;
    }
    if (plength)
      *plength = length;
    if (!(*pdata = (char *)malloc(length))) {
      close(fd);
      return ERRMEM;
    }
    n = http_read_buffer(fd, *pdata, length);
    close(fd);
    if (n != length)
      ret = ERRRDDT;
  } else if (ret >= 0)
    close(fd);
  return ret;
}

/*
 * Request the header
 *
 * This function outputs the header of thehttp data server.
 * The header is from the ressource named filename.
 * The length and type of data is eventually returned (like for http_get(3))
 *
 * returns a negative error code or a positive code from the server
 *
 * limitations: filename is truncated to first 256 characters
 */
HttpRequest::http_retcode HttpRequest::http_head(
    const char *filename, /* name of the ressource to read */
    int *plength,   /* address of integer variable which will be set to
                       length of the data */
    char *typebuf   /* allocated buffer where the data type is returned.
                     If NULL, the type is not returned */
)
{
  /* mostly copied from http_get : */
  http_retcode ret;

  char header[MAXBUF];
  char *pc;
  int fd;
  int n, length = -1;

  if (plength)
    *plength = 0;
  if (typebuf)
    *typebuf = '\0';

  ret = http_query("HEAD", filename, "", KEEP_OPEN, NULL, 0, &fd);
  if (ret == 200) {
    while (1) {
      n = http_read_line(fd, header, MAXBUF - 1);
#ifdef VERBOSE
      fputs(header, stderr);
      putc('\n', stderr);
#endif
      if (n <= 0) {
        close(fd);
        return ERRRDHD;
      }
      /* empty line ? (=> end of header) */
      if (n > 0 && (*header) == '\0')
        break;
      /* try to parse some keywords : */
      /* convert to lower case 'till a : is found or end of string */
      for (pc = header; (*pc != ':' && *pc); pc++)
        *pc = tolower(*pc);
      sscanf(header, "content-length: %d", &length);
      if (typebuf)
        sscanf(header, "content-type: %s", typebuf);
    }
    if (plength)
      *plength = length;
    close(fd);
  } else if (ret >= 0)
    close(fd);
  return ret;
}

/*
 * Delete data on the server
 *
 * This function request a DELETE on the http data server.
 *
 * returns a negative error code or a positive code from the server
 *
 * limitations: filename is truncated to first 256 characters
 */
HttpRequest::http_retcode
HttpRequest::http_delete(const char *filename /* name of the ressource to create */)
{ return http_query("DELETE", filename, "", CLOSE, NULL, 0, NULL); }

/* parses an url : setting the http_server and http_port global variables
 * and returning the filename to pass to http_get/put/...
 * returns a negative error code or 0 if sucessfully parsed.
 */
HttpRequest::http_retcode HttpRequest::http_parse_url(
    /* writeable copy of an url */
    char *url,
    /* address of a pointer that will be filled with allocated filename
     * the pointer must be equal to NULL before calling or it will be
     * automatically freed (free(3))
     */
    char **pfilename)
{
  char *pc, c;

  http_port = 80;
  http_server.clear();
  if (*pfilename) {
    free(*pfilename);
    *pfilename = NULL;
  }

  if (strncasecmp("http://", url, 7)) {
#ifdef VERBOSE
    fprintf(stderr, "invalid url (must start with 'http://')\n");
#endif
    return ERRURLH;
  }
  url += 7;
  for (pc = url, c = *pc; (c && c != ':' && c != '/');)
    c = *pc++;
  *(pc - 1) = 0;
  if (c == ':') {
    if (sscanf(pc, "%d", &http_port) != 1) {
#ifdef VERBOSE
      fprintf(stderr, "invalid port in url\n");
#endif
      return ERRURLP;
    }
    for (pc++; (*pc && *pc != '/'); pc++)
      ;
    if (*pc)
      pc++;
  }

  http_server = url;
  *pfilename = strdup(c ? pc : "");

#ifdef VERBOSE
  fprintf(stderr, "host=(%s), port=%d, filename=(%s)\n", http_server.c_str(),
          http_port, *pfilename);
#endif
  return OK0;
}

HttpRequest::http_retcode HttpRequest::set_url(const std::string &url)
{
  fname.clear();
  char *filename = nullptr;
  char *url_c_str = strdup(url.c_str());
  auto ret = http_parse_url(url_c_str, &filename);
  free(url_c_str);
  fname = filename;
  free(filename);
  return ret;
}

std::string HttpRequest::get()
{
  std::string data_str;
  char *data = nullptr;
  char typebuf[70];
  int lg;
  auto ret = http_get(fname.c_str(), &data, &lg, typebuf);
  if (ret == 200)
    data_str.assign(data, lg);
  free(data);
  return data_str;
}

