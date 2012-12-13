/**
 * Workaround header wrapper for bison parsers to allow
 * compatibility to older versions of bison not supporting
 * the '%code requires' option.
 */
#include "common.h"
#include "interpreter.h"
#include "expressions.h"
#include "parameters.h"
#include "statements.h"
#include "groups.h"
#include "patterns.h"
#include "ast.h"
#ifdef HAVE_MPI
  #include <mpi.h>
#endif

#include "parser.tab.h"
