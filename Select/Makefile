# Non de l'executable
EXENAME = tftp

# Compilateur
CC = gcc

# Repertoires
INCDIR = include
SRCDIR = src
DEPDIR = depend
OBJDIR = obj
BINDIR = bin

# Executable (target par defaut)
EXE = bin/$(EXENAME)

# Liste des fichiers sources, et fchiers objets et dependances correspondants
SRCFILES = $(wildcard $(SRCDIR)/*.c)
OBJFILES = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCFILES))
DEPFILES = $(patsubst $(SRCDIR)/%.c,$(DEPDIR)/%.d,$(SRCFILES))

# Chemin de recherche des headers
INCPATH = -I$(INCDIR)

# Options de compilation
COPTS = -Wall

# Flags de compilation
CFLAGS = $(COPTS) $(INCPATH)

# Chemin de recherche des librairies
LIBPATH =
	
# Librairies a linker avec l'executable
LIBS = -lm

# Flags d'edition des liens
LDFLAGS = $(LIBPATH) $(LIBS)


#----------------------------------------------------------------------------------------------------------------------
# TARGETS 
#----------------------------------------------------------------------------------------------------------------------

# Target par defaut (genere l'executable)
exe: $(EXE)

$(EXE): $(OBJFILES)
	gcc $(OBJFILES) $(LDFLAGS) -o $(EXE)

# Include dependencies
-include $(DEPFILES)

# Source file compilation rule
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo '> Compilation de : $<'
	$(CC) -c $(CFLAGS) -MMD -MP -MF"$(DEPDIR)/$*.d" -o $@ $<
	@echo '< Fin de la compilation : $<'
	@echo

# Nettoyage
clean:
	$(RM) $(OBJFILES)
	$(RM) $(DEPFILES)
	$(RM) $(EXE)

