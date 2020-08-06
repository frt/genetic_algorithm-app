CC=$(MPICC)
AM_CPPFLAGS = -I$(prefix)/include
bin_PROGRAMS = genetic_algorithm-app
genetic_algorithm_app_SOURCES = main.c fitness.c
genetic_algorithm_app_LDADD = -L$(libdir) -Ltopology_parser \
			       -lparallel_evolution \
			       -lgaul \
			       -lgaul_util \
			       -ltopology_parser \
			       -lm
			       
SUBDIRS = topology_parser
