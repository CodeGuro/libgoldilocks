include $(top_srcdir)/variables.am

noinst_PROGRAMS = goldilocks_gen_tables

goldilocks_gen_tables_SOURCES = utils.c \
					   goldilocks_gen_tables.c \
					   arch_x86_64/f_impl.c \
	       			   f_arithmetic.c \
	       			   f_generic.c \
	      			   goldilocks.c \
	      			   scalar.c

goldilocks_gen_tables_CFLAGS = $(AM_CFLAGS) $(LANGFLAGS) $(WARNFLAGS) $(INCFLAGS) $(INCFLAGS_448) $(OFLAGS) $(ARCHFLAGS) $(GENFLAGS) $(XCFLAGS)
goldilocks_gen_tables_LDFLAGS = $(AM_LDFLAGS) $(XLDFLAGS)


GEN/decaf_tables.c: goldilocks_gen_tables
	./$< > $@ || (rm $@; exit 1)

lib_LTLIBRARIES = libgoldilocks.la

libgoldilocks_la_SOURCES = utils.c \
		      shake.c \
		      spongerng.c \
		      arch_x86_64/f_impl.c \
		      f_arithmetic.c \
		      f_generic.c \
		      goldilocks.c \
		      elligator.c \
		      scalar.c \
		      eddsa.c \
		      GEN/decaf_tables.c

libgoldilocks_la_CFLAGS = $(AM_CFLAGS) $(LANGFLAGS) $(WARNFLAGS) $(INCFLAGS) $(OFLAGS) $(ARCHFLAGS) $(GENFLAGS) $(XCFLAGS)
libgoldilocks_la_LDFLAGS = $(AM_LDFLAGS) $(XLDFLAGS)

incsubdir = $(includedir)/goldilocks

incsub_HEADERS = public_include/goldilocks/common.h \
		 public_include/goldilocks/ed448.h \
		 public_include/goldilocks/ed448.hxx \
		 public_include/goldilocks/eddsa.hxx \
		 public_include/goldilocks/point_448.h \
		 public_include/goldilocks/point_448.hxx \
		 public_include/goldilocks/secure_buffer.hxx \
		 public_include/goldilocks/shake.h \
		 public_include/goldilocks/shake.hxx \
		 public_include/goldilocks/spongerng.h \
		 public_include/goldilocks/spongerng.hxx

include_HEADERS = public_include/goldilocks.h \
		  public_include/goldilocks.hxx

#pkginclude_HEADERS = public_include/

#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = libgoldilocks.pc
