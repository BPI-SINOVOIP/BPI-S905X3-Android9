/* Compress or decompress an ELF file.
   Copyright (C) 2015 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>
#include <assert.h>
#include <argp.h>
#include <error.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include ELFUTILS_HEADER(elf)
#include ELFUTILS_HEADER(ebl)
#include <gelf.h>
#include "system.h"

/* Name and version of program.  */
static void print_version (FILE *stream, struct argp_state *state);
ARGP_PROGRAM_VERSION_HOOK_DEF = print_version;

/* Bug report address.  */
ARGP_PROGRAM_BUG_ADDRESS_DEF = PACKAGE_BUGREPORT;

static int verbose = 0; /* < 0, no warnings, > 0 extra verbosity.  */
static bool force = false;
static bool permissive = false;
static const char *foutput = NULL;

#define T_UNSET 0
#define T_DECOMPRESS 1    /* none */
#define T_COMPRESS_ZLIB 2 /* zlib */
#define T_COMPRESS_GNU  3 /* zlib-gnu */
static int type = T_UNSET;

static void
print_version (FILE *stream, struct argp_state *state __attribute__ ((unused)))
{
  fprintf (stream, "elfcompress (%s) %s\n", PACKAGE_NAME, PACKAGE_VERSION);
}

struct section_pattern
{
  char *pattern;
  struct section_pattern *next;
};

static struct section_pattern *patterns = NULL;

static void
add_pattern (const char *pattern)
{
  struct section_pattern *p = xmalloc (sizeof *p);
  p->pattern = xstrdup (pattern);
  p->next = patterns;
  patterns = p;
}

static void
free_patterns (void)
{
  struct section_pattern *pattern = patterns;
  while (pattern != NULL)
    {
      struct section_pattern *p = pattern;
      pattern = p->next;
      free (p->pattern);
      free (p);
    }
}

static error_t
parse_opt (int key, char *arg __attribute__ ((unused)),
	   struct argp_state *state __attribute__ ((unused)))
{
  switch (key)
    {
    case 'v':
      verbose++;
      break;

    case 'q':
      verbose--;

    case 'f':
      force = true;
      break;

    case 'p':
      permissive = true;
      break;

    case 'n':
      add_pattern (arg);
      break;

    case 'o':
      if (foutput != NULL)
	argp_error (state, N_("-o option specified twice"));
      else
	foutput = arg;
      break;

    case 't':
      if (type != T_UNSET)
	argp_error (state, N_("-t option specified twice"));

      if (strcmp ("none", arg) == 0)
	type = T_DECOMPRESS;
      else if (strcmp ("zlib", arg) == 0 || strcmp ("zlib-gabi", arg) == 0)
	type = T_COMPRESS_ZLIB;
      else if (strcmp ("zlib-gnu", arg) == 0 || strcmp ("gnu", arg) == 0)
	type = T_COMPRESS_GNU;
      else
	argp_error (state, N_("unknown compression type '%s'"), arg);
      break;

    case ARGP_KEY_SUCCESS:
      if (type == T_UNSET)
	type = T_COMPRESS_ZLIB;
      if (patterns == NULL)
	add_pattern (".?(z)debug*");
      break;

    case ARGP_KEY_NO_ARGS:
      /* We need at least one input file.  */
      argp_error (state, N_("No input file given"));
      break;

    case ARGP_KEY_ARGS:
      if (foutput != NULL && state->argc - state->next > 1)
	argp_error (state,
		    N_("Only one input file allowed together with '-o'"));
      /* We only use this for checking the number of arguments, we don't
	 actually want to consume them, so fallthrough.  */
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static bool
section_name_matches (const char *name)
{
  struct section_pattern *pattern = patterns;
  while (pattern != NULL)
    {
      if (fnmatch (pattern->pattern, name, FNM_EXTMATCH) == 0)
	return true;
      pattern = pattern->next;
    }
  return false;
}

static int
setshdrstrndx (Elf *elf, GElf_Ehdr *ehdr, size_t ndx)
{
  if (ndx < SHN_LORESERVE)
    ehdr->e_shstrndx = ndx;
  else
    {
      ehdr->e_shstrndx = SHN_XINDEX;
      Elf_Scn *zscn = elf_getscn (elf, 0);
      GElf_Shdr zshdr_mem;
      GElf_Shdr *zshdr = gelf_getshdr (zscn, &zshdr_mem);
      if (zshdr == NULL)
	return -1;
      zshdr->sh_link = ndx;
      if (gelf_update_shdr (zscn, zshdr) == 0)
	return -1;
    }

  if (gelf_update_ehdr (elf, ehdr) == 0)
    return -1;

  return 0;
}

static int
compress_section (Elf_Scn *scn, size_t orig_size, const char *name,
		  const char *newname, size_t ndx,
		  bool gnu, bool compress, bool report_verbose)
{
  int res;
  unsigned int flags = compress && force ? ELF_CHF_FORCE : 0;
  if (gnu)
    res = elf_compress_gnu (scn, compress ? 1 : 0, flags);
  else
    res = elf_compress (scn, compress ? ELFCOMPRESS_ZLIB : 0, flags);

  if (res < 0)
    error (0, 0, "Couldn't decompress section [%zd] %s: %s",
	   ndx, name, elf_errmsg (-1));
  else
    {
      if (compress && res == 0)
	{
	  if (verbose >= 0)
	    printf ("[%zd] %s NOT compressed, wouldn't be smaller\n",
		    ndx, name);
	}

      if (report_verbose && res > 0)
	{
	  printf ("[%zd] %s %s", ndx, name,
		  compress ? "compressed" : "decompressed");
	  if (newname != NULL)
	    printf (" -> %s", newname);

	  /* Reload shdr, it has changed.  */
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  if (shdr == NULL)
	    {
	      error (0, 0, "Couldn't get shdr for section [%zd]", ndx);
	      return -1;
	    }
	  float new = shdr->sh_size;
	  float orig = orig_size ?: 1;
	  printf (" (%zu => %" PRIu64 " %.2f%%)\n",
		  orig_size, shdr->sh_size, (new / orig) * 100);
	}
    }

  return res;
}

static int
process_file (const char *fname)
{
  if (verbose > 0)
    printf ("processing: %s\n", fname);

  /* The input ELF.  */
  int fd = -1;
  Elf *elf = NULL;

  /* The output ELF.  */
  char *fnew = NULL;
  int fdnew = -1;
  Elf *elfnew = NULL;

  /* Buffer for (one) new section name if necessary.  */
  char *snamebuf = NULL;

  /* String table (and symbol table), if section names need adjusting.  */
  struct Ebl_Strtab *names = NULL;
  struct Ebl_Strent **scnstrents = NULL;
  struct Ebl_Strent **symstrents = NULL;
  char **scnnames = NULL;

  /* Section data from names.  */
  void *namesbuf = NULL;

  /* Which sections match and need to be (un)compressed.  */
  unsigned int *sections = NULL;

  /* How many sections are we talking about?  */
  size_t shnum = 0;

#define WORD_BITS (8U * sizeof (unsigned int))
  void set_section (size_t ndx)
  {
    sections[ndx / WORD_BITS] |= (1U << (ndx % WORD_BITS));
  }

  bool get_section (size_t ndx)
  {
    return (sections[ndx / WORD_BITS] & (1U << (ndx % WORD_BITS))) != 0;
  }

  int cleanup (int res)
  {
    elf_end (elf);
    close (fd);

    elf_end (elfnew);
    close (fdnew);

    if (fnew != NULL)
      {
	unlink (fnew);
	free (fnew);
	fnew = NULL;
      }

    free (snamebuf);
    if (names != NULL)
      {
	ebl_strtabfree (names);
	free (scnstrents);
	free (symstrents);
	free (namesbuf);
	if (scnnames != NULL)
	  {
	    for (size_t n = 0; n < shnum; n++)
	      free (scnnames[n]);
	    free (scnnames);
	  }
      }

    free (sections);

    return res;
  }

  fd = open (fname, O_RDONLY);
  if (fd < 0)
    {
      error (0, errno, "Couldn't open %s\n", fname);
      return cleanup (-1);
    }

  elf = elf_begin (fd, ELF_C_READ, NULL);
  if (elf == NULL)
    {
      error (0, 0, "Couldn't open ELF file %s for reading: %s",
	     fname, elf_errmsg (-1));
      return cleanup (-1);
    }

  /* We dont' handle ar files (or anything else), we probably should.  */
  Elf_Kind kind = elf_kind (elf);
  if (kind != ELF_K_ELF)
    {
      if (kind == ELF_K_AR)
	error (0, 0, "Cannot handle ar files: %s", fname);
      else
	error (0, 0, "Unknown file type: %s", fname);
      return cleanup (-1);
    }

  struct stat st;
  if (fstat (fd, &st) != 0)
    {
      error (0, errno, "Couldn't fstat %s", fname);
      return cleanup (-1);
    }

  GElf_Ehdr ehdr;
  if (gelf_getehdr (elf, &ehdr) == NULL)
    {
      error (0, 0, "Couldn't get ehdr for %s: %s", fname, elf_errmsg (-1));
      return cleanup (-1);
    }

  /* Get the section header string table.  */
  size_t shdrstrndx;
  if (elf_getshdrstrndx (elf, &shdrstrndx) != 0)
    {
      error (0, 0, "Couldn't get section header string table index in %s: %s",
	     fname, elf_errmsg (-1));
      return cleanup (-1);
    }

  /* How many sections are we talking about?  */
  if (elf_getshdrnum (elf, &shnum) != 0)
    {
      error (0, 0, "Couldn't get number of sections in %s: %s",
	     fname, elf_errmsg (1));
      return cleanup (-1);
    }

  if (shnum == 0)
    {
      error (0, 0, "ELF file %s has no sections", fname);
      return cleanup (-1);
    }

  sections = xcalloc (shnum / 8 + 1, sizeof (unsigned int));

  size_t phnum;
  if (elf_getphdrnum (elf, &phnum) != 0)
    {
      error (0, 0, "Couldn't get phdrnum: %s", elf_errmsg (-1));
      return cleanup (-1);
    }

  /* Whether we need to adjust any section names (going to/from GNU
     naming).  If so we'll need to build a new section header string
     table.  */
  bool adjust_names = false;

  /* If there are phdrs we want to maintain the layout of the
     allocated sections in the file.  */
  bool layout = phnum != 0;

  /* While going through all sections keep track of last section data
     offset if needed to keep the layout.  We are responsible for
     adding the section offsets and headers (e_shoff) in that case
     (which we will place after the last section).  */
  GElf_Off last_offset = 0;
  if (layout)
    last_offset = (ehdr.e_phoff
		   + gelf_fsize (elf, ELF_T_PHDR, phnum, EV_CURRENT));

  /* Which section, if any, is a symbol table that shares a string
     table with the section header string table?  */
  size_t symtabndx = 0;

  /* We do three passes over all sections.

     First an inspection pass over the old Elf to see which section
     data needs to be copied and/or transformed, which sections need a
     names change and whether there is a symbol table that might need
     to be adjusted be if the section header name table is changed.

     Second a collection pass that creates the Elf sections and copies
     the data.  This pass will compress/decompress section data when
     needed.  And it will collect all data needed if we'll need to
     construct a new string table. Afterwards the new string table is
     constructed.

     Third a fixup/adjustment pass over the new Elf that will adjust
     any section references (names) and adjust the layout based on the
     new sizes of the sections if necessary.  This pass is optional if
     we aren't responsible for the layout and the section header
     string table hasn't been changed.  */

  /* Inspection pass.  */
  size_t maxnamelen = 0;
  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (elf, scn)) != NULL)
    {
      size_t ndx = elf_ndxscn (scn);
      if (ndx > shnum)
	{
	  error (0, 0, "Unexpected section number %zd, expected only %zd",
		 ndx, shnum);
	  cleanup (-1);
	}

      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      if (shdr == NULL)
	{
	  error (0, 0, "Couldn't get shdr for section %zd", ndx);
	  return cleanup (-1);
	}

      const char *sname = elf_strptr (elf, shdrstrndx, shdr->sh_name);
      if (sname == NULL)
	{
	  error (0, 0, "Couldn't get name for section %zd", ndx);
	  return cleanup (-1);
	}

      if (section_name_matches (sname))
	{
	  if (shdr->sh_type != SHT_NOBITS
	      && (shdr->sh_flags & SHF_ALLOC) == 0)
	    {
	      set_section (ndx);
	      /* Check if we might want to change this section name.  */
	      if (! adjust_names
		  && ((type != T_COMPRESS_GNU
		       && strncmp (sname, ".zdebug",
				   strlen (".zdebug")) == 0)
		      || (type == T_COMPRESS_GNU
			  && strncmp (sname, ".debug",
				      strlen (".debug")) == 0)))
		adjust_names = true;

	      /* We need a buffer this large if we change the names.  */
	      if (adjust_names)
		{
		  size_t slen = strlen (sname);
		  if (slen > maxnamelen)
		    maxnamelen = slen;
		}
	    }
	  else
	    if (verbose >= 0)
	      printf ("[%zd] %s ignoring %s section\n", ndx, sname,
		      (shdr->sh_type == SHT_NOBITS ? "no bits" : "allocated"));
	}

      if (shdr->sh_type == SHT_SYMTAB)
	{
	  /* Check if we might have to adjust the symbol name indexes.  */
	  if (shdr->sh_link == shdrstrndx)
	    {
	      if (symtabndx != 0)
		{
		  error (0, 0,
			 "Multiple symbol tables (%zd, %zd) using the same string table unsupported", symtabndx, ndx);
		  return cleanup (-1);
		}
	      symtabndx = ndx;
	    }
	}

      /* Keep track of last allocated data offset.  */
      if (layout)
	if ((shdr->sh_flags & SHF_ALLOC) != 0)
	  {
	    GElf_Off off = shdr->sh_offset + (shdr->sh_type != SHT_NOBITS
					      ? shdr->sh_size : 0);
	    if (last_offset < off)
	      last_offset = off;
	  }
    }

  if (adjust_names)
    {
      names = ebl_strtabinit (true);
      if (names == NULL)
	{
	  error (0, 0, "Not enough memory for new strtab");
	  return cleanup (-1);
	}
      scnstrents = xmalloc (shnum
			    * sizeof (struct Ebl_Strent *));
      scnnames = xcalloc (shnum, sizeof (char *));
    }

  /* Create a new (temporary) ELF file for the result.  */
  if (foutput == NULL)
    {
      size_t fname_len = strlen (fname);
      fnew = xmalloc (fname_len + sizeof (".XXXXXX"));
      strcpy (mempcpy (fnew, fname, fname_len), ".XXXXXX");
      fdnew = mkstemp (fnew);
    }
  else
    {
      fnew = xstrdup (foutput);
      fdnew = open (fnew, O_WRONLY | O_CREAT, st.st_mode & ALLPERMS);
    }

  if (fdnew < 0)
    {
      error (0, errno, "Couldn't create output file %s", fnew);
      /* Since we didn't create it we don't want to try to unlink it.  */
      free (fnew);
      fnew = NULL;
      return cleanup (-1);
    }

  elfnew = elf_begin (fdnew, ELF_C_WRITE, NULL);
  if (elfnew == NULL)
    {
      error (0, 0, "Couldn't open new ELF %s for writing: %s",
	     fnew, elf_errmsg (-1));
      return cleanup (-1);
    }

  /* Create the new ELF header and copy over all the data.  */
  if (gelf_newehdr (elfnew, gelf_getclass (elf)) == 0)
    {
      error (0, 0, "Couldn't create new ehdr: %s", elf_errmsg (-1));
      return cleanup (-1);
    }

  GElf_Ehdr newehdr;
  if (gelf_getehdr (elfnew, &newehdr) == NULL)
    {
      error (0, 0, "Couldn't get new ehdr: %s", elf_errmsg (-1));
      return cleanup (-1);
    }

  newehdr.e_ident[EI_DATA] = ehdr.e_ident[EI_DATA];
  newehdr.e_type = ehdr.e_type;
  newehdr.e_machine = ehdr.e_machine;
  newehdr.e_version = ehdr.e_version;
  newehdr.e_entry = ehdr.e_entry;
  newehdr.e_flags = ehdr.e_flags;

  if (gelf_update_ehdr (elfnew, &newehdr) == 0)
    {
      error (0, 0, "Couldn't update ehdr: %s", elf_errmsg (-1));
      return cleanup (-1);
    }

  /* Copy over the phdrs as is.  */
  if (phnum != 0)
    {
      if (gelf_newphdr (elfnew, phnum) == 0)
	{
	  error (0, 0, "Couldn't create phdrs: %s", elf_errmsg (-1));
	  return cleanup (-1);
	}

      for (size_t cnt = 0; cnt < phnum; ++cnt)
	{
	  GElf_Phdr phdr_mem;
	  GElf_Phdr *phdr = gelf_getphdr (elf, cnt, &phdr_mem);
	  if (phdr == NULL)
	    {
	      error (0, 0, "Couldn't get phdr %zd: %s", cnt, elf_errmsg (-1));
	      return cleanup (-1);
	    }
	  if (gelf_update_phdr (elfnew, cnt, phdr) == 0)
	    {
	      error (0, 0, "Couldn't create phdr %zd: %s", cnt,
		     elf_errmsg (-1));
	      return cleanup (-1);
	    }
	}
    }

  /* Possibly add a 'z' and zero terminator.  */
  if (maxnamelen > 0)
    snamebuf = xmalloc (maxnamelen + 2);

  /* We might want to read/adjust the section header strings and
     symbol tables.  If so, and those sections are to be compressed
     then we will have to decompress it during the collection pass and
     compress it again in the fixup pass.  Don't compress unnecessary
     and keep track of whether or not to compress them (later in the
     fixup pass).  Also record the original size, so we can report the
     difference later when we do compress.  */
  int shstrtab_compressed = T_UNSET;
  size_t shstrtab_size = 0;
  char *shstrtab_name = NULL;
  char *shstrtab_newname = NULL;
  int symtab_compressed = T_UNSET;
  size_t symtab_size = 0;
  char *symtab_name = NULL;
  char *symtab_newname = NULL;

  /* Collection pass.  Copy over the sections, (de)compresses matching
     sections, collect names of sections and symbol table if
     necessary.  */
  scn = NULL;
  while ((scn = elf_nextscn (elf, scn)) != NULL)
    {
      size_t ndx = elf_ndxscn (scn);
      assert (ndx < shnum);

      /* (de)compress if section matched.  */
      char *sname = NULL;
      char *newname = NULL;
      if (get_section (ndx))
	{
	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  if (shdr == NULL)
	    {
	      error (0, 0, "Couldn't get shdr for section %zd", ndx);
	      return cleanup (-1);
	    }

	  uint64_t size = shdr->sh_size;
	  sname = elf_strptr (elf, shdrstrndx, shdr->sh_name);
	  if (sname == NULL)
	    {
	      error (0, 0, "Couldn't get name for section %zd", ndx);
	      return cleanup (-1);
	    }

	  /* strdup sname, the shdrstrndx section itself might be
	     (de)compressed, invalidating the string pointers.  */
	  sname = xstrdup (sname);

	  /* We might want to decompress (and rename), but not
	     compress during this pass since we might need the section
	     data in later passes.  Skip those sections for now and
	     compress them in the fixup pass.  */
	  bool skip_compress_section = (adjust_names
					&& (ndx == shdrstrndx
					    || ndx == symtabndx));

	  switch (type)
	    {
	    case T_DECOMPRESS:
	      if ((shdr->sh_flags & SHF_COMPRESSED) != 0)
		{
		  if (compress_section (scn, size, sname, NULL, ndx,
					false, false, verbose > 0) < 0)
		    return cleanup (-1);
		}
	      else if (strncmp (sname, ".zdebug", strlen (".zdebug")) == 0)
		{
		  snamebuf[0] = '.';
		  strcpy (&snamebuf[1], &sname[2]);
		  newname = snamebuf;
		  if (compress_section (scn, size, sname, newname, ndx,
					true, false, verbose > 0) < 0)
		    return cleanup (-1);
		}
	      else if (verbose > 0)
		printf ("[%zd] %s already decompressed\n", ndx, sname);
	      break;

	    case T_COMPRESS_GNU:
	      if (strncmp (sname, ".debug", strlen (".debug")) == 0)
		{
		  if ((shdr->sh_flags & SHF_COMPRESSED) != 0)
		    {
		      /* First decompress to recompress GNU style.
			 Don't report even when verbose.  */
		      if (compress_section (scn, size, sname, NULL, ndx,
					    false, false, false) < 0)
			return cleanup (-1);
		    }

		  snamebuf[0] = '.';
		  snamebuf[1] = 'z';
		  strcpy (&snamebuf[2], &sname[1]);
		  newname = snamebuf;

		  if (skip_compress_section)
		    {
		      if (ndx == shdrstrndx)
			{
			  shstrtab_size = size;
			  shstrtab_compressed = T_COMPRESS_GNU;
			  shstrtab_name = xstrdup (sname);
			  shstrtab_newname = xstrdup (newname);
			}
		      else
			{
			  symtab_size = size;
			  symtab_compressed = T_COMPRESS_GNU;
			  symtab_name = xstrdup (sname);
			  symtab_newname = xstrdup (newname);
			}
		    }
		  else
		    {
		      int res = compress_section (scn, size, sname, newname,
						  ndx, true, true,
						  verbose > 0);
		      if (res < 0)
			return cleanup (-1);

		      if (res == 0)
			newname = NULL;
		    }
		}
	      else if (verbose >= 0)
		{
		  if (strncmp (sname, ".zdebug", strlen (".zdebug")) == 0)
		    printf ("[%zd] %s unchanged, already GNU compressed",
			    ndx, sname);
		  else
		    printf ("[%zd] %s cannot GNU compress section not starting with .debug\n",
			    ndx, sname);
		}
	      break;

	    case T_COMPRESS_ZLIB:
	      if ((shdr->sh_flags & SHF_COMPRESSED) == 0)
		{
		  if (strncmp (sname, ".zdebug", strlen (".zdebug")) == 0)
		    {
		      /* First decompress to recompress zlib style.
			 Don't report even when verbose.  */
		      if (compress_section (scn, size, sname, NULL, ndx,
					    true, false, false) < 0)
			return cleanup (-1);

		      snamebuf[0] = '.';
		      strcpy (&snamebuf[1], &sname[2]);
		      newname = snamebuf;
		    }

		  if (skip_compress_section)
		    {
		      if (ndx == shdrstrndx)
			{
			  shstrtab_size = size;
			  shstrtab_compressed = T_COMPRESS_ZLIB;
			  shstrtab_name = xstrdup (sname);
			  shstrtab_newname = (newname == NULL
					      ? NULL : xstrdup (newname));
			}
		      else
			{
			  symtab_size = size;
			  symtab_compressed = T_COMPRESS_ZLIB;
			  symtab_name = xstrdup (sname);
			  symtab_newname = (newname == NULL
					    ? NULL : xstrdup (newname));
			}
		    }
		  else if (compress_section (scn, size, sname, newname, ndx,
					     false, true, verbose > 0) < 0)
		    return cleanup (-1);
		}
	      else if (verbose > 0)
		printf ("[%zd] %s already compressed\n", ndx, sname);
	      break;
	    }

	  free (sname);
	}

      Elf_Scn *newscn = elf_newscn (elfnew);
      if (newscn == NULL)
	{
	  error (0, 0, "Couldn't create new section %zd", ndx);
	  return cleanup (-1);
	}

      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      if (shdr == NULL)
	{
	  error (0, 0, "Couldn't get shdr for section %zd", ndx);
	  return cleanup (-1);
	}

      if (gelf_update_shdr (newscn, shdr) == 0)
        {
	  error (0, 0, "Couldn't update section header %zd", ndx);
	  return cleanup (-1);
	}

      /* Except for the section header string table all data can be
	 copied as is.  The section header string table will be
	 created later and the symbol table might be fixed up if
	 necessary.  */
      if (! adjust_names || ndx != shdrstrndx)
	{
	  Elf_Data *data = elf_getdata (scn, NULL);
	  if (data == NULL)
	    {
	      error (0, 0, "Couldn't get data from section %zd", ndx);
	      return cleanup (-1);
	    }

	  Elf_Data *newdata = elf_newdata (newscn);
	  if (newdata == NULL)
	    {
	      error (0, 0, "Couldn't create new data for section %zd", ndx);
	      return cleanup (-1);
	    }

	  *newdata = *data;
	}

      /* Keep track of the (new) section names.  */
      if (adjust_names)
	{
	  char *name;
	  if (newname != NULL)
	    name = newname;
	  else
	    {
	      name = elf_strptr (elf, shdrstrndx, shdr->sh_name);
	      if (name == NULL)
		{
		  error (0, 0, "Couldn't get name for section [%zd]", ndx);
		  return cleanup (-1);
		}
	    }

	  /* We need to keep a copy of the name till the strtab is done.  */
	  name = scnnames[ndx] = xstrdup (name);
	  if ((scnstrents[ndx] = ebl_strtabadd (names, name, 0)) == NULL)
	    {
	      error (0, 0, "No memory to add section name string table");
	      return cleanup (-1);
	    }

	  /* If the symtab shares strings then add those too.  */
	  if (ndx == symtabndx)
	    {
	      /* If the section is (still) compressed we'll need to
		 uncompress it first to adjust the data, then
		 recompress it in the fixup pass.  */
	      if (symtab_compressed == T_UNSET)
		{
		  size_t size = shdr->sh_size;
		  if ((shdr->sh_flags == SHF_COMPRESSED) != 0)
		    {
		      /* Don't report the (internal) uncompression.  */
		      if (compress_section (newscn, size, sname, NULL, ndx,
					    false, false, false) < 0)
			return cleanup (-1);

		      symtab_size = size;
		      symtab_compressed = T_COMPRESS_ZLIB;
		    }
		  else if (strncmp (name, ".zdebug", strlen (".zdebug")) == 0)
		    {
		      /* Don't report the (internal) uncompression.  */
		      if (compress_section (newscn, size, sname, NULL, ndx,
					    true, false, false) < 0)
			return cleanup (-1);

		      symtab_size = size;
		      symtab_compressed = T_COMPRESS_GNU;
		    }
		}

	      Elf_Data *symd = elf_getdata (newscn, NULL);
	      if (symd == NULL)
		{
		  error (0, 0, "Couldn't get symtab data for section [%zd] %s",
			 ndx, name);
		  return cleanup (-1);
		}
	      size_t elsize = gelf_fsize (elfnew, ELF_T_SYM, 1, EV_CURRENT);
	      size_t syms = symd->d_size / elsize;
	      symstrents = xmalloc (syms * sizeof (struct Ebl_Strent *));
	      for (size_t i = 0; i < syms; i++)
		{
		  GElf_Sym sym_mem;
		  GElf_Sym *sym = gelf_getsym (symd, i, &sym_mem);
		  if (sym == NULL)
		    {
		      error (0, 0, "Couldn't get symbol %zd", i);
		      return cleanup (-1);
		    }
		  if (sym->st_name != 0)
		    {
		      /* Note we take the name from the original ELF,
			 since the new one will not have setup the
			 strtab yet.  */
		      const char *symname = elf_strptr (elf, shdrstrndx,
							sym->st_name);
		      if (symname == NULL)
			{
			  error (0, 0, "Couldn't get symbol %zd name", i);
			  return cleanup (-1);
			}
		      symstrents[i] = ebl_strtabadd (names, symname, 0);
		      if (symstrents[i] == NULL)
			{
			  error (0, 0, "No memory to add to symbol name");
			  return cleanup (-1);
			}
		    }
		}
	    }
	}
    }

  if (adjust_names)
    {
      /* We got all needed strings, put the new data in the shstrtab.  */
      if (verbose > 0)
	printf ("[%zd] Updating section string table\n", shdrstrndx);

      scn = elf_getscn (elfnew, shdrstrndx);
      if (scn == NULL)
	{
	  error (0, 0, "Couldn't get new section header string table [%zd]",
		 shdrstrndx);
	  return cleanup (-1);
	}

      Elf_Data *data = elf_newdata (scn);
      if (data == NULL)
	{
	  error (0, 0, "Couldn't create new section header string table data");
	  return cleanup (-1);
	}
      ebl_strtabfinalize (names, data);
      namesbuf = data->d_buf;

      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
      if (shdr == NULL)
	{
	  error (0, 0, "Couldn't get shdr for new section strings %zd",
		 shdrstrndx);
	  return cleanup (-1);
	}

      /* Note that we also might have to compress and possibly set
	 sh_off below */
      shdr->sh_name = ebl_strtaboffset (scnstrents[shdrstrndx]);
      shdr->sh_type = SHT_STRTAB;
      shdr->sh_flags = 0;
      shdr->sh_addr = 0;
      shdr->sh_offset = 0;
      shdr->sh_size = data->d_size;
      shdr->sh_link = SHN_UNDEF;
      shdr->sh_info = SHN_UNDEF;
      shdr->sh_addralign = 1;
      shdr->sh_entsize = 0;

      if (gelf_update_shdr (scn, shdr) == 0)
	{
	  error (0, 0, "Couldn't update new section strings [%zd]",
		 shdrstrndx);
	  return cleanup (-1);
	}

      /* We might have to compress the data if the user asked us to,
	 or if the section was already compressed (and the user didn't
	 ask for decompression).  Note somewhat identical code for
	 symtab below.  */
      if (shstrtab_compressed == T_UNSET)
	{
	  /* The user didn't ask for compression, but maybe it was
	     compressed in the original ELF file.  */
	  Elf_Scn *oldscn = elf_getscn (elf, shdrstrndx);
	  if (oldscn == NULL)
	    {
	      error (0, 0, "Couldn't get section header string table [%zd]",
		     shdrstrndx);
	      return cleanup (-1);
	    }

	  shdr = gelf_getshdr (oldscn, &shdr_mem);
	  if (shdr == NULL)
	    {
	      error (0, 0, "Couldn't get shdr for old section strings [%zd]",
		     shdrstrndx);
	      return cleanup (-1);
	    }

	  shstrtab_name = elf_strptr (elf, shdrstrndx, shdr->sh_name);
	  if (shstrtab_name == NULL)
	    {
	      error (0, 0, "Couldn't get name for old section strings [%zd]",
		     shdrstrndx);
	      return cleanup (-1);
	    }

	  shstrtab_size = shdr->sh_size;
	  if ((shdr->sh_flags & SHF_COMPRESSED) != 0)
	    shstrtab_compressed = T_COMPRESS_ZLIB;
	  else if (strncmp (shstrtab_name, ".zdebug", strlen (".zdebug")) == 0)
	    shstrtab_compressed = T_COMPRESS_GNU;
	}

      /* Should we (re)compress?  */
      if (shstrtab_compressed != T_UNSET)
	{
	  if (compress_section (scn, shstrtab_size, shstrtab_name,
				shstrtab_newname, shdrstrndx,
				shstrtab_compressed == T_COMPRESS_GNU,
				true, verbose > 0) < 0)
	    return cleanup (-1);
	}
    }

  /* Make sure to re-get the new ehdr.  Adding phdrs and shdrs will
     have changed it.  */
  if (gelf_getehdr (elfnew, &newehdr) == NULL)
    {
      error (0, 0, "Couldn't re-get new ehdr: %s", elf_errmsg (-1));
      return cleanup (-1);
    }

  /* Set this after the sections have been created, otherwise section
     zero might not exist yet.  */
  if (setshdrstrndx (elfnew, &newehdr, shdrstrndx) != 0)
    {
      error (0, 0, "Couldn't set new shdrstrndx: %s", elf_errmsg (-1));
      return cleanup (-1);
    }

  /* Fixup pass.  Adjust string table references, symbol table and
     layout if necessary.  */
  if (layout || adjust_names)
    {
      scn = NULL;
      while ((scn = elf_nextscn (elfnew, scn)) != NULL)
	{
	  size_t ndx = elf_ndxscn (scn);

	  GElf_Shdr shdr_mem;
	  GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
	  if (shdr == NULL)
	    {
	      error (0, 0, "Couldn't get shdr for section %zd", ndx);
	      return cleanup (-1);
	    }

	  /* Keep the offset of allocated sections so they are at the
	     same place in the file. Add (possibly changed)
	     unallocated ones after the allocated ones.  */
	  if ((shdr->sh_flags & SHF_ALLOC) == 0)
	    {
	      /* Zero means one.  No alignment constraints.  */
	      size_t addralign = shdr->sh_addralign ?: 1;
	      last_offset = (last_offset + addralign - 1) & ~(addralign - 1);
	      shdr->sh_offset = last_offset;
	      if (shdr->sh_type != SHT_NOBITS)
		last_offset += shdr->sh_size;
	    }

	  if (adjust_names)
	    shdr->sh_name = ebl_strtaboffset (scnstrents[ndx]);

	  if (gelf_update_shdr (scn, shdr) == 0)
	    {
	      error (0, 0, "Couldn't update section header %zd", ndx);
	      return cleanup (-1);
	    }

	  if (adjust_names && ndx == symtabndx)
	    {
	      if (verbose > 0)
		printf ("[%zd] Updating symbol table\n", symtabndx);

	      Elf_Data *symd = elf_getdata (scn, NULL);
	      if (symd == NULL)
		{
		  error (0, 0, "Couldn't get new symtab data section [%zd]",
			 ndx);
		  return cleanup (-1);
		}
	      size_t elsize = gelf_fsize (elfnew, ELF_T_SYM, 1, EV_CURRENT);
	      size_t syms = symd->d_size / elsize;
	      for (size_t i = 0; i < syms; i++)
		{
		  GElf_Sym sym_mem;
		  GElf_Sym *sym = gelf_getsym (symd, i, &sym_mem);
		  if (sym == NULL)
		    {
		      error (0, 0, "2 Couldn't get symbol %zd", i);
		      return cleanup (-1);
		    }

		  if (sym->st_name != 0)
		    {
		      sym->st_name = ebl_strtaboffset (symstrents[i]);

		      if (gelf_update_sym (symd, i, sym) == 0)
			{
			  error (0, 0, "Couldn't update symbol %zd", i);
			  return cleanup (-1);
			}
		    }
		}

	      /* We might have to compress the data if the user asked
		 us to, or if the section was already compressed (and
		 the user didn't ask for decompression).  Note
		 somewhat identical code for shstrtab above.  */
	      if (symtab_compressed == T_UNSET)
		{
		  /* The user didn't ask for compression, but maybe it was
		     compressed in the original ELF file.  */
		  Elf_Scn *oldscn = elf_getscn (elf, symtabndx);
		  if (oldscn == NULL)
		    {
		      error (0, 0, "Couldn't get symbol table [%zd]",
			     symtabndx);
		      return cleanup (-1);
		    }

		  shdr = gelf_getshdr (oldscn, &shdr_mem);
		  if (shdr == NULL)
		    {
		      error (0, 0, "Couldn't get old symbol table shdr [%zd]",
			     symtabndx);
		      return cleanup (-1);
		    }

		  symtab_name = elf_strptr (elf, shdrstrndx, shdr->sh_name);
		  if (symtab_name == NULL)
		    {
		      error (0, 0, "Couldn't get old symbol table name [%zd]",
			     symtabndx);
		      return cleanup (-1);
		    }

		  symtab_size = shdr->sh_size;
		  if ((shdr->sh_flags & SHF_COMPRESSED) != 0)
		    symtab_compressed = T_COMPRESS_ZLIB;
		  else if (strncmp (symtab_name, ".zdebug",
				    strlen (".zdebug")) == 0)
		    symtab_compressed = T_COMPRESS_GNU;
		}

	      /* Should we (re)compress?  */
	      if (symtab_compressed != T_UNSET)
		{
		  if (compress_section (scn, symtab_size, symtab_name,
					symtab_newname, symtabndx,
					symtab_compressed == T_COMPRESS_GNU,
					true, verbose > 0) < 0)
		    return cleanup (-1);
		}
	    }
	}
    }

  /* If we have phdrs we want elf_update to layout the SHF_ALLOC
     sections precisely as in the original file.  In that case we are
     also responsible for setting phoff and shoff */
  if (layout)
    {
      if (gelf_getehdr (elfnew, &newehdr) == NULL)
	{
	  error (0, 0, "Couldn't get ehdr: %s", elf_errmsg (-1));
	  return cleanup (-1);
	}

      /* Position the shdrs after the last (unallocated) section.  */
      const size_t offsize = gelf_fsize (elfnew, ELF_T_OFF, 1, EV_CURRENT);
      newehdr.e_shoff = ((last_offset + offsize - 1)
			 & ~((GElf_Off) (offsize - 1)));

      /* The phdrs go in the same place as in the original file.
	 Normally right after the ELF header.  */
      newehdr.e_phoff = ehdr.e_phoff;

      if (gelf_update_ehdr (elfnew, &newehdr) == 0)
	{
	  error (0, 0, "Couldn't update ehdr: %s", elf_errmsg (-1));
	  return cleanup (-1);
	}
    }

  elf_flagelf (elfnew, ELF_C_SET, ((layout ? ELF_F_LAYOUT : 0)
				   | (permissive ? ELF_F_PERMISSIVE : 0)));

  if (elf_update (elfnew, ELF_C_WRITE) < 0)
    {
      error (0, 0, "Couldn't write %s: %s", fnew, elf_errmsg (-1));
      return cleanup (-1);
    }

  elf_end (elfnew);
  elfnew = NULL;

  /* Try to match mode and owner.group of the original file.  */
  if (fchmod (fdnew, st.st_mode & ALLPERMS) != 0)
    if (verbose >= 0)
      error (0, errno, "Couldn't fchmod %s", fnew);
  if (fchown (fdnew, st.st_uid, st.st_gid) != 0)
    if (verbose >= 0)
      error (0, errno, "Couldn't fchown %s", fnew);

  /* Finally replace the old file with the new file.  */
  if (foutput == NULL)
    if (rename (fnew, fname) != 0)
      {
	error (0, errno, "Couldn't rename %s to %s", fnew, fname);
	return cleanup (-1);
      }

  /* We are finally done with the new file, don't unlink it now.  */
  free (fnew);
  fnew = NULL;

  return cleanup (0);
}

int
main (int argc, char **argv)
{
  const struct argp_option options[] =
    {
      { "output", 'o', "FILE", 0,
	N_("Place (de)compressed output into FILE"),
	0 },
      { "type", 't', "TYPE", 0,
	N_("What type of compression to apply. TYPE can be 'none' (decompress), 'zlib' (ELF ZLIB compression, the default, 'zlib-gabi' is an alias) or 'zlib-gnu' (.zdebug GNU style compression, 'gnu' is an alias)"),
	0 },
      { "name", 'n', "SECTION", 0,
	N_("SECTION name to (de)compress, SECTION is an extended wildcard pattern (defaults to '.?(z)debug*')"),
	0 },
      { "verbose", 'v', NULL, 0,
	N_("Print a message for each section being (de)compressed"),
	0 },
      { "force", 'f', NULL, 0,
	N_("Force compression of section even if it would become larger"),
	0 },
      { "permissive", 'p', NULL, 0,
	N_("Relax a few rules to handle slightly broken ELF files"),
	0 },
      { "quiet", 'q', NULL, 0,
	N_("Be silent when a section cannot be compressed"),
	0 },
      { NULL, 0, NULL, 0, NULL, 0 }
    };

  const struct argp argp =
    {
      .options = options,
      .parser = parse_opt,
      .args_doc = N_("FILE..."),
      .doc = N_("Compress or decompress sections in an ELF file.")
    };

  int remaining;
  if (argp_parse (&argp, argc, argv, 0, &remaining, NULL) != 0)
    return EXIT_FAILURE;

  /* Should already be handled by ARGP_KEY_NO_ARGS case above,
     just sanity check.  */
  if (remaining >= argc)
    error (EXIT_FAILURE, 0, N_("No input file given"));

  /* Likewise for the ARGP_KEY_ARGS case above, an extra sanity check.  */
  if (foutput != NULL && remaining + 1 < argc)
    error (EXIT_FAILURE, 0,
	   N_("Only one input file allowed together with '-o'"));

  elf_version (EV_CURRENT);

  /* Process all the remaining files.  */
  int result = 0;
  do
    result |= process_file (argv[remaining]);
  while (++remaining < argc);

  free_patterns ();
  return result;
}
