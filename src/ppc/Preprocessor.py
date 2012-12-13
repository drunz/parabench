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
Brick           -->  A template consists of several bricks.
Brick Instance  -->  A set of option values for a certain brick.
                     This is used for repeating bricks with different options in a template.

The template file system structure is as follows:
<template_name.tpl>   -->  Template body file.
<template_name>       -->  Brick files for this template.
    <brick_name>.tpl  -->  A brick template.
    ...
"""

from TemplateEngine import *
from CodeGenerator import *
from ModuleProcessor import *

import glob,os, sys, inspect

#
# Constants
#
VERSION = '0.0.5'
VERBOSE = False

# Source File to Hook bindings
hooks = {}
hooks['scanner.l']       = ['scanner_keyword']
hooks['parser.y']        = ['parser_token', 'parser_identifier']
hooks['statements.h']    = ['statement_enum']
hooks['interpreter.c']   = ['statement_exec', 'module_include']

# Hook to Generator bindings
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
def run():
    print '*** Parabench Module Preprocessor', VERSION, '***'
    
    #callPath = os.path.dirname(inspect.getfile(sys._getframe()))+'/'
    
    for moduleFile in glob.glob('modules/*.c'):
        module = Module(moduleFile);
        if VERBOSE:
            module.printParameters()
        for sourceFile in hooks:
            cg = CodeGenerator(sourceFile)
            for hook in hooks[sourceFile]:
                print 'ppc: %s -> %s -> %s' % (moduleFile, sourceFile, hook)
                if generator[hook]:
                    g = generator[hook]()
                    template = Template(hook)
                    g.generate(template, module)
                else:
                    print 'No generator given for hook "%s"' % hook
            cg.write()
    
    
    # Copy production source files to gen dir:
    genfiles = hooks.keys()
    srcfiles = glob.glob('*.c') + glob.glob('*.h') + glob.glob('*.l') + glob.glob('*.y')
    #print genfiles
    #print srcfiles
    for g in genfiles:
        if g in srcfiles:
            srcfiles.remove(g)
    #print srcfiles
    
    source = ' '.join(srcfiles)
    if source != '':
        os.system('cp %s gen/' % (' '.join(srcfiles)))
    
    #print '\n\nGenerated Code:'
    #cg.printCode()

if __name__ == "__main__":
    run()
