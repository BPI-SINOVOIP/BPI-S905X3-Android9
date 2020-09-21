/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _CUPS_CONFIG_H_
#define _CUPS_CONFIG_H_

#define CUPS_SVERSION "CUPS v2.2.6"
#define CUPS_MINIMAL "CUPS/2.2.6"
#define CUPS_DEFAULT_PRINTOPERATOR_AUTH "@SYSTEM"
#define CUPS_DEFAULT_LOG_LEVEL "warn"
#define CUPS_DEFAULT_BROWSE_LOCAL_PROTOCOLS "dnssd"
#define CUPS_DEFAULT_IPP_PORT 631
#define CUPS_DEFAULT_DOMAINSOCKET "/private/var/run/cupsd"
#define CUPS_DATADIR "/usr/share/cups"
#define CUPS_LOCALEDIR "/usr/share/locale"
#define CUPS_SERVERBIN "/usr/lib/cups"
#define CUPS_SERVERROOT "/etc/cups"
#define CUPS_STATEDIR "/var/run/cups"
#define HAVE_LIBZ 1
#define HAVE_STDINT_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_LONG_LONG 1
#ifdef HAVE_LONG_LONG
#  define CUPS_LLFMT "%lld"
#  define CUPS_LLCAST (long long)
#else
#  define CUPS_LLFMT "%ld"
#  define CUPS_LLCAST (long)
#endif /* HAVE_LONG_LONG */
#define HAVE_STRDUP 1
#define HAVE_GETEUID 1
#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF 1
#define HAVE_SIGACTION 1
#define HAVE_SECIDENTITYSEARCHCREATEWITHPOLICY 1
#define HAVE_CSSMERRORSTRING 1
#define HAVE_GETADDRINFO 1
#define HAVE_GETNAMEINFO 1
#define HAVE_HSTRERROR 1
#undef HAVE_RES_INIT
#define HAVE_RESOLV_H 1
#undef HAVE_PTHREAD_H
#define CUPS_DEFAULT_GSSSERVICENAME "host"
#define HAVE_POLL 1
#define CUPS_RAND() random()
#define CUPS_SRAND(v) srandom(v)
#define HAVE_SSL 1

#endif /* !_CUPS_CONFIG_H_ */
