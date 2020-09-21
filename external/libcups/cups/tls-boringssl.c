/*
 * TLS support code for CUPS using Google BoringSSL.
 *
 * Copyright 2007-2016 by Apple Inc.
 * Copyright 1997-2007 by Easy Software Products, all rights reserved.
 *
 * These coded instructions, statements, and computer programs are the
 * property of Apple Inc. and are protected by Federal copyright
 * law.  Distribution and use rights are outlined in the file "LICENSE.txt"
 * which should have been included with this file.  If this file is
 * file is missing or damaged, see the license at "http://www.cups.org/".
 *
 * This file is subject to the Apple OS-Developed Software exception.
 */

/**** This file is included from tls.c ****/

/*
 * Local globals...
 */

#include "cups-private.h"
#include "http.h"
#include "thread-private.h"
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <sys/stat.h>

static char		*tls_keypath = NULL;
					/* Server cert keychain path */
static int		tls_options = -1;/* Options for TLS connections */


/*
 * Local functions...
 */

static BIO_METHOD *     _httpBIOMethods(void);
static int              http_bio_write(BIO *h, const char *buf, int num);
static int              http_bio_read(BIO *h, char *buf, int size);
static int              http_bio_puts(BIO *h, const char *str);
static long             http_bio_ctrl(BIO *h, int cmd, long arg1, void *arg2);
static int              http_bio_new(BIO *h);
static int              http_bio_free(BIO *data);

static BIO_METHOD	http_bio_methods =
			{
			  BIO_TYPE_SSL,
			  "http",
			  http_bio_write,
			  http_bio_read,
			  http_bio_puts,
			  NULL, /* http_bio_gets, */
			  http_bio_ctrl,
			  http_bio_new,
			  http_bio_free,
			  NULL,
			};

/*
 * 'cupsMakeServerCredentials()' - Make a self-signed certificate and private key pair.
 *
 * @since CUPS 2.0/OS 10.10@
 */

int					/* O - 1 on success, 0 on failure */
cupsMakeServerCredentials(
    const char *path,			/* I - Path to keychain/directory */
    const char *common_name,		/* I - Common name */
    int        num_alt_names,		/* I - Number of subject alternate names */
    const char **alt_names,		/* I - Subject Alternate Names */
    time_t     expiration_date)		/* I - Expiration date */
{
  int		pid,			/* Process ID of command */
		status;			/* Status of command */
  char		command[1024],		/* Command */
		*argv[12],		/* Command-line arguments */
		*envp[1000],	        /* Environment variables */
		infofile[1024],		/* Type-in information for cert */
		seedfile[1024];		/* Random number seed file */
  int		envc,			/* Number of environment variables */
		bytes;			/* Bytes written */
  cups_file_t	*fp;			/* Seed/info file */
  int		infofd;			/* Info file descriptor */
  char          temp[1024],	        /* Temporary directory name */
                crtfile[1024],	        /* Certificate filename */
                keyfile[1024];	        /* Private key filename */

  DEBUG_printf(("cupsMakeServerCredentials(path=\"%s\", common_name=\"%s\", num_alt_names=%d, alt_names=%p, expiration_date=%d)", path, common_name, num_alt_names, alt_names, (int)expiration_date));

  return 0;
}


/*
 * '_httpCreateCredentials()' - Create credentials in the internal format.
 */

http_tls_credentials_t			/* O - Internal credentials */
_httpCreateCredentials(
    cups_array_t *credentials)		/* I - Array of credentials */
{
  (void)credentials;

  return (NULL);
}


/*
 * '_httpFreeCredentials()' - Free internal credentials.
 */

void
_httpFreeCredentials(
    http_tls_credentials_t credentials)	/* I - Internal credentials */
{
  (void)credentials;
}


/*
 * '_httpBIOMethods()' - Get the OpenSSL BIO methods for HTTP connections.
 */

static BIO_METHOD *                            /* O - BIO methods for OpenSSL */
_httpBIOMethods(void)
{
  return (&http_bio_methods);
}


/*
 * 'http_bio_ctrl()' - Control the HTTP connection.
 */

static long				/* O - Result/data */
http_bio_ctrl(BIO  *h,			/* I - BIO data */
              int  cmd,			/* I - Control command */
	      long arg1,		/* I - First argument */
	      void *arg2)		/* I - Second argument */
{
  switch (cmd)
  {
    default :
        return (0);

    case BIO_CTRL_RESET :
        h->ptr = NULL;
	return (0);

    case BIO_C_SET_FILE_PTR :
        h->ptr  = arg2;
	h->init = 1;
	return (1);

    case BIO_C_GET_FILE_PTR :
        if (arg2)
	{
	  *((void **)arg2) = h->ptr;
	  return (1);
	}
	else
	  return (0);

    case BIO_CTRL_DUP :
    case BIO_CTRL_FLUSH :
        return (1);
  }
}


/*
 * 'http_bio_free()' - Free OpenSSL data.
 */

static int				/* O - 1 on success, 0 on failure */
http_bio_free(BIO *h)			/* I - BIO data */
{
  if (!h)
    return (0);

  if (h->shutdown)
  {
    h->init  = 0;
    h->flags = 0;
  }

  return (1);
}


/*
 * 'http_bio_new()' - Initialize an OpenSSL BIO structure.
 */

static int				/* O - 1 on success, 0 on failure */
http_bio_new(BIO *h)			/* I - BIO data */
{
  if (!h)
    return (0);

  h->init  = 0;
  h->num   = 0;
  h->ptr   = NULL;
  h->flags = 0;

  return (1);
}


/*
 * 'http_bio_puts()' - Send a string for OpenSSL.
 */

static int				/* O - Bytes written */
http_bio_puts(BIO        *h,		/* I - BIO data */
              const char *str)		/* I - String to write */
{
  return (send(((http_t *)h->ptr)->fd, str, strlen(str), 0));
}


/*
 * 'http_bio_read()' - Read data for OpenSSL.
 */

static int				/* O - Bytes read */
http_bio_read(BIO  *h,			/* I - BIO data */
              char *buf,		/* I - Buffer */
	      int  size)		/* I - Number of bytes to read */
{
  http_t	*http;			/* HTTP connection */


  http = (http_t *)h->ptr;

  if (!http->blocking)
  {
   /*
    * Make sure we have data before we read...
    */

    while (!_httpWait(http, http->wait_value, 0))
    {
      if (http->timeout_cb && (*http->timeout_cb)(http, http->timeout_data))
	continue;

      http->error = ETIMEDOUT;

      return (-1);
    }
  }

  return (recv(http->fd, buf, size, 0));
}


/*
 * 'http_bio_write()' - Write data for OpenSSL.
 */

static int				/* O - Bytes written */
http_bio_write(BIO        *h,		/* I - BIO data */
               const char *buf,		/* I - Buffer to write */
	       int        num)		/* I - Number of bytes to write */
{
  return (send(((http_t *)h->ptr)->fd, buf, num, 0));
}


/*
 * '_httpTLSInitialize()' - Initialize the TLS stack.
 */

void
_httpTLSInitialize(void)
{
  SSL_library_init();
}


/*
 * '_httpTLSPending()' - Return the number of pending TLS-encrypted bytes.
 */

size_t					/* O - Bytes available */
_httpTLSPending(http_t *http)		/* I - HTTP connection */
{
  return (SSL_pending(http->tls));
}


/*
 * '_httpTLSRead()' - Read from a SSL/TLS connection.
 */

int					/* O - Bytes read */
_httpTLSRead(http_t *http,		/* I - Connection to server */
	     char   *buf,		/* I - Buffer to store data */
	     int    len)		/* I - Length of buffer */
{
  return (SSL_read((SSL *)(http->tls), buf, len));
}


/*
 * '_httpTLSSetOptions()' - Set TLS protocol and cipher suite options.
 */

void
_httpTLSSetOptions(int options)		/* I - Options */
{
  tls_options = options;
}


/*
 * '_httpTLSStart()' - Set up SSL/TLS support on a connection.
 */

int					/* O - 0 on success, -1 on failure */
_httpTLSStart(http_t *http)		/* I - Connection to server */
{
  char			hostname[256],	/* Hostname */
			*hostptr;	/* Pointer into hostname */

  SSL_CTX		*context;	/* Context for encryption */
  BIO			*bio;		/* BIO data */
  const char		*message = NULL;/* Error message */

  DEBUG_printf(("3_httpTLSStart(http=%p)", (void *)http));

  if (tls_options < 0)
  {
    DEBUG_puts("4_httpTLSStart: Setting defaults.");
    _cupsSetDefaults();
    DEBUG_printf(("4_httpTLSStart: tls_options=%x", tls_options));
  }

  if (http->mode == _HTTP_MODE_SERVER && !tls_keypath)
  {
    DEBUG_puts("4_httpTLSStart: cupsSetServerCredentials not called.");
    http->error  = errno = EINVAL;
    http->status = HTTP_STATUS_ERROR;
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, _("Server credentials not set."), 1);

    return (-1);
  }

  context = SSL_CTX_new(TLS_method());
  if (tls_options & _HTTP_TLS_DENY_TLS10)
    SSL_CTX_set_min_proto_version(context, TLS1_1_VERSION);

  bio = BIO_new(_httpBIOMethods());
  BIO_ctrl(bio, BIO_C_SET_FILE_PTR, 0, (char *)http);

  http->tls = SSL_new(context);
  SSL_set_bio(http->tls, bio, bio);

  /* http->tls retains an internal reference to the SSL_CTX. */
  SSL_CTX_free(context);

  if (http->mode == _HTTP_MODE_CLIENT)
  {
    SSL_set_connect_state(http->tls);

   /*
    * Client: get the hostname to use for TLS...
    */

    if (httpAddrLocalhost(http->hostaddr))
    {
      strlcpy(hostname, "localhost", sizeof(hostname));
    }
    else
    {
     /*
      * Otherwise make sure the hostname we have does not end in a trailing dot.
      */

      strlcpy(hostname, http->hostname, sizeof(hostname));
      if ((hostptr = hostname + strlen(hostname) - 1) >= hostname &&
	  *hostptr == '.')
	*hostptr = '\0';
    }
    SSL_set_tlsext_host_name(http->tls, hostname);
  }
  else
  {
/* @@@ TODO @@@ */
    _cupsSetError(IPP_STATUS_ERROR_INTERNAL, "Server not supported", 0);
  }


  if (SSL_do_handshake(http->tls) != 1)
  {
    unsigned long	error;	/* Error code */
    char		buf[256];

    while ((error = ERR_get_error()) != 0)
    {
      ERR_error_string_n(error, buf, sizeof(buf));
      DEBUG_printf(("8http_setup_ssl: %s", buf));
    }

    SSL_free(http->tls);
    http->tls = NULL;

    http->error  = errno;
    http->status = HTTP_STATUS_ERROR;

    if (!message)
      message = _("Unable to establish a secure connection to host.");

    _cupsSetError(IPP_STATUS_ERROR_CUPS_PKI, message, 1);

    return (-1);
  }

  return (0);
}


/*
 * '_httpTLSStop()' - Shut down SSL/TLS on a connection.
 */

void
_httpTLSStop(http_t *http)		/* I - Connection to server */
{
  unsigned long	error;			/* Error code */

  switch (SSL_shutdown(http->tls))
  {
    case 1 :
	break;

    case -1 :
        _cupsSetError(IPP_STATUS_ERROR_INTERNAL,
			"Fatal error during SSL shutdown!", 0);
    default :
	while ((error = ERR_get_error()) != 0)
        {
	  char	buf[256];
	  ERR_error_string_n(error, buf, sizeof(buf));
	  _cupsSetError(IPP_STATUS_ERROR_INTERNAL, buf, 0);
        }
	break;
  }

  SSL_free(http->tls);
  http->tls = NULL;
}

/*
 * '_httpTLSWrite()' - Write to a SSL/TLS connection.
 */

int					/* O - Bytes written */
_httpTLSWrite(http_t     *http,		/* I - Connection to server */
	      const char *buf,		/* I - Buffer holding data */
	      int        len)		/* I - Length of buffer */
{
  int	result;			/* Return value */


  DEBUG_printf(("2http_write_ssl(http=%p, buf=%p, len=%d)", http, buf, len));

  result = SSL_write((SSL *)(http->tls), buf, len);

  DEBUG_printf(("3http_write_ssl: Returning %d.", result));

  return result;
}
