#!/usr/bin/env python

# Parabench - A parallel file system benchmark
# Copyright (C) 2009-2010  Dennis Runz
# University of Heidelberg
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
Parabench Module Template System:

Hook            -->  Marks a point in source code file where to insert generated code.
Template        -->  Code template with placeholders used to generated code.
Brick           -->  A building block for pieces of code. A template consists of several bricks.
                     Bricks are templates themselves used for generating sub-blocks of code within a parent template.
Brick Instance  -->  A set of key-value pairs for a certain brick.
                     Brick instances are used to generate final code blocks using bricks.

The template file system structure is as follows:
<template_name.tpl>   -->  Template body file, which may include optional bricks.
<template_name>       -->  Brick files for this template.
    <brick_name>.tpl  -->  A brick template.
    ...
"""

from Template import Template
from CodeGenerator import CodeGenerator, ScannerKeywordGenerator, ParserTokenGenerator, ParserIdentifierGenerator
from CodeGenerator import StatementEnumGenerator, StatementExecGenerator, ModuleIncludeGenerator
from ModuleProcessor import Module

import glob, os #, sys, inspect

#
# Constants
#
VERSION = '0.0.5'
VERBOSE = False

# Source File to Hook bindings
#    Each source file can have several 'hooks' which marks
#    the place where to insert which kind of generated code.
hooks = {}
hooks['scanner.l']       = ['scanner_keyword']
hooks['parser.y']        = ['parser_token', 'parser_identifier']
hooks['statements.h']    = ['statement_enum']
hooks['interpreter.c']   = ['statement_exec', 'module_include']

# Hook to Generator bindings
#    Each hook has a specific generator object, which knows
#    how to generate the code specific for this hook.
generator = {}
generator['scanner_keyword']   = ScannerKeywordGenerator
generator['parser_token']      = ParserTokenGenerator
generator['parser_identifier'] = ParserIdentifierGenerator
generator['statement_enum']    = StatementEnumGenerator
generator['statement_exec']    = StatementExecGenerator
generator['module_include']    = ModuleIncludeGenerator

# TODO: Allow dynamic parameters for a module: ParameterList* pList

#
# Main
#
def main():
    print '*** Parabench Module Preprocessor', VERSION, '***'
    
    #callPath = os.path.dirname(inspect.getfile(sys._getframe()))+'/'
    
    for module_file in glob.glob('modules/*.c'):
        module = Module(module_file);
        if VERBOSE:
            module.print_parameters()
        for source_file in hooks:
            cg = CodeGenerator(source_file)
            for hook in hooks[source_file]:
                print 'ppc: %s -> %s -> %s' % (module_file, source_file, hook)
                if generator[hook]:
                    g = generator[hook]()
                    template = Template(hook)
                    g.generate(template, module)
                else:
                    print 'No generator given for hook "%s"' % hook
            cg.write()
    
    
    # Copy production source files to gen dir:
    genfiles = hooks.keys()
    srcfiles = set(glob.glob('*.c') + glob.glob('*.h') + glob.glob('*.l') + glob.glob('*.y'))
    
    for g in genfiles:
        if g in srcfiles:
            srcfiles.remove(g)
    
    source = ' '.join(srcfiles)
    if source != '':
        os.system('cp %s gen/' % source)
    
    if VERBOSE:
        print '\n\nGenerated Code:'
        cg.print_code()


if __name__ == "__main__":
    main()
