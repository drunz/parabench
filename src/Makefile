SRC = ast.c expressions.c parameters.c statements.c interpreter.c memfunc.c iofunc.c timing.c
DEP = ast.c ast.h expressions.c expressions.h parameters.c parameters.h statements.c statements.h interpreter.c interpreter.h memfunc.c memfunc.h iofunc.c iofunc.h timing.c timing.h Makefile
TESTDIR = test

all: parser parabench

test-report: test/test_expressions.c test/test_posixio.c $(DEP)
		@echo 'Building target: $@'
		mpicc `pkg-config --libs --cflags glib-2.0` -lm -O0 -Wall -ggdb $(TESTDIR)/test_expressions.c $(SRC) -o $(TESTDIR)/test_expressions
		mpicc `pkg-config --libs --cflags glib-2.0` -lm -O0 -Wall -ggdb $(TESTDIR)/test_posixio.c $(SRC) -o $(TESTDIR)/test_posixio
		@echo 'Finished building target: $@'
		@echo ' '
		
		@echo 'Running regression tests...'
		cd $(TESTDIR); \
		mkdir results; \
		gtester -k -o results.xml test_expressions test_posixio; \
		gtester-report results.xml > results/`date +%d%b%G_%H%M%S`.html; \
		rm results.xml
		@echo 'Test report created.'
		@echo ' '

parser: kulga.l kulga.y Makefile
		@echo 'Building target: $@'
		# Debug
#		flex -d -p -i -B -o scanner.c kulga.lex
		# Normal/Standard
#		flex -i -B -o scanner.c kulga.lex
		# Fast/Optimized tables
		flex -CFr -i -B --yylineno kulga.l
		bison --debug --report=all --locations -d -g kulga.y
		mv lex.yy.c kulga.lex.c
		@echo 'Finished building target: $@'
		@echo ' '
		
parser-graph:
		@echo 'Building target: $@'
		dot -Tpng -ograph.png kulga.dot
		@echo 'Finished building target: $@'
		@echo ' '

expressions: expressions.c Makefile
		@echo 'Building target: $@'
		# Debug
		gcc `pkg-config --libs --cflags glib-2.0` -lm -O0 -Wall -ggdb -g3 -fstack-protector-all expressions.c -o $@
		# Release
		#mpicc `pkg-config --libs --cflags glib-2.0` -lm -O2 -Wall scanner.c parser.c interpreter.c memfunc.c iofunc.c bstrlib.c main.c -o $@
		@echo 'Finished building target: $@'
		@echo ' '
		
parabench: ast.c ast.h expressions.c expressions.h parameters.c parameters.h statements.c statements.h interpreter.c interpreter.h memfunc.c memfunc.h iofunc.c iofunc.h main.c config.h kulga.l kulga.y Makefile
		@echo 'Building target: $@'
		# Debug
		mpicc `pkg-config --libs --cflags glib-2.0` -DYYDEBUG=1 -D_VERBOSE -lm -O0 -Wall -ggdb ast.c expressions.c parameters.c statements.c kulga.lex.c kulga.tab.c interpreter.c memfunc.c iofunc.c main.c -o $@ 
#		mpicc `pkg-config --libs --cflags glib-2.0` -DNDEBUG -lm -O2 ast.c expressions.c parameters.c statements.c kulga.lex.c kulga.tab.c interpreter.c memfunc.c iofunc.c main.c -o $@
#		mpicc `pkg-config --libs --cflags glib-2.0` -lm -O2 ast.c expressions.c parameters.c statements.c kulga.lex.c kulga.tab.c interpreter.c memfunc.c iofunc.c main.c -o $@
		# Release
		#mpicc `pkg-config --libs --cflags glib-2.0` -lm -O2 -Wall scanner.c parser.c interpreter.c memfunc.c iofunc.c bstrlib.c main.c -o $@
		@echo 'Finished building target: $@'
		@echo ' '
