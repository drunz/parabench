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

import re


class SourceWriter:
    """Writes generated code strings into source files"""
    
    _hook_map = {}
    
    def __init__(self, source_file):
        if source_file:
            self._source_file = source_file
            self._target_file = self._gen_file_suffix(source_file)
            self.pure_generator = False
        else:
            self.pure_generator = True
        
        hook_format = r'^[ ]*[\t]*(/\*)[ ]*(!\[ModuleHook\])[ ]+(?P<hook_name>[A-Za-z0-9_]+)[ ]*(\*/)'
        self._hook_prog = re.compile(hook_format)
    
    def _gen_file_suffix(self, file_name):
        return 'gen/' + file_name
#        if '/' in file_name:
#            parts = file_name.rsplit('/', 1)
#            return parts[0]+'/gen/'+parts[1]
#        else:
#            return file_name + '.gen'
    
    def _add_code_for_hook(self, hook, code):
        if hook in self._hook_map:
            self._hook_map[hook] += code
        else:
            self._hook_map[hook]  = code
    
    def write(self):
        """Write the generated code to files."""
        if not self.pure_generator:
            source = open(self._source_file, 'r')
            target = open(self._target_file, 'w')
            
            # Iterate over source lines and insert code at hook if found
            for line in source:
                # Write the code to the found hook
                m = self._hook_prog.match(line)
                if m:
                    # Note: The spaces in the BEGIN/END comments are important!
                    #       Without the leading spaces, flex will interpet the comments as rules
                    #       and complain about "unrecognized rule".
                    hook_name = m.group('hook_name')
                    target.write('  /* BEGIN - Automatically generated code. Do not change! */\n')
                    target.write(self._hook_map[hook_name])
                    target.write('  /* END - Automatically generated code. Do not change! */\n')
                else:
                    target.write(line)
    
            source.close()
            target.close()
        else:
            raise TypeError('write() called on pure generator.\n \
            Pure generators cannot be used to write code.\n \
            Initialize with a source file Name.')


class CodeGenerator(SourceWriter):
    """Abstract class that defines interfaces for code generators
    that allow to produce final source code files using templates.
    For each type of code block to be generated, a specific
    code generator subclass is defined."""
    
    stmt_enum_prefix = 'STMT_MOD_'
    parser_token_prefix = 'TMOD_'
    param_fetch_map = {'gchar*': 'param_string_get', 
                       'glong': 'param_int_get'}

    def __init__(self, source_file=None):
        """Passing None as source_file will create a pure generator,
        which is not able to write code. If a source file name is given,
        the generator searches for hooks in this file and replaces them
        with already generated code."""
        SourceWriter.__init__(self, source_file)
    
    def print_code(self):
        for key, value in self._hook_map.iteritems():
            print '[%s]\n' % key
            print value
            print '\n\n'
    
    def generate(self, template, module):
        """Generate code for the specified module using the given template.
        The generated code integrates the module into the current code base at compile time.
        
        Abstract method: Use any of the provided child classes to generate code
        for an integration hook."""
        raise TypeError('Abstract method called')

            
class StatementExecGenerator(CodeGenerator):
    """Implements generator routines for producing
    code blocks responsible for executing interpreter statements.
    
    Relevant template: statement_exec.tpl"""
    
    def __init__(self, source_file=None):
        CodeGenerator.__init__(self, source_file)
        
    def _generate_body_brick_instance(self, module):
        # properties
        enum_type = self.stmt_enum_prefix + module.function_name.upper()
        num_param = len(module.parameter_list)
        
        # generate
        brick_instance = {}
        brick_instance['<statement_enum_type>'] = enum_type
        brick_instance['<num_param>'] = str(num_param)
        return brick_instance
    
    def _generate_fetch_brick_instances(self, module):
        # properties
        num_param = len(module.parameter_list)
        parameter_list = module.parameter_list
        
        # generate
        instance_list = []
        brick_instance = {}
        for i in range(num_param):
            param_type = parameter_list[i][0]
            param_name = parameter_list[i][1]
            brick_instance = {"<param_type>": param_type, 
                              "<param_name>": param_name, 
                              "<param_fetch_call>": self.param_fetch_map[param_type], 
                              "<param_id>": str(i)}
            instance_list.append(brick_instance)
        return instance_list
    
    def _generate_assert_brick_instances(self, module):
        # properties
        num_param = len(module.parameter_list)
        
        # generate
        instance_list = []
        brick_instance = {'<num_param>':str(num_param)}
        
        instance_list.append(brick_instance)
        return instance_list
    
    def _generate_call_brick_instances(self, module):
        # properties
        parameterList = ', '.join(map(lambda x: x[1], module.parameter_list))
        enumType = self.stmt_enum_prefix + module.function_name.upper()
        
        # generate
        instance_list = []
        brick_instance = {'<io_func_call>':module.function_name,
                         '<io_func_param_list>':parameterList,
                         '<statement_enum_type>':enumType}
        
        instance_list.append(brick_instance)
        return instance_list
    
    def generate(self, template, module):
        if template.name() != 'statement_exec' and not module.valid():
            raise NameError('Cannot handle integration hook "%s"' % template.name())
        
        brick = {}
        brick['body']             = self._generate_body_brick_instance(module)
        brick['parameter_fetch']  = self._generate_fetch_brick_instances(module)
        brick['parameter_assert'] = self._generate_assert_brick_instances(module)
        brick['module_call']      = self._generate_call_brick_instances(module)
        
        code = template.process(brick)
        self._add_code_for_hook('statement_exec', code)


class StatementEnumGenerator(CodeGenerator):
    """Implements generator routines for producing
    code blocks responsible for statement definition enums.
    
    Relevant template: statement_enum.tpl"""
    
    def __init__(self, source_file=None):
        CodeGenerator.__init__(self, source_file)
        
    def _generate_body_brick_instance(self, module):
        # properties
        enumType = self.stmt_enum_prefix + module.function_name.upper()
        
        # generate
        brick_instance = {}
        brick_instance['<statement_enum_type>'] = enumType
        return brick_instance
    
    def generate(self, template, module):
        if template.name() != 'statement_enum' and not module.valid():
            raise NameError('Cannot handle integration hook "%s"' % template.name())
        
        brick = {}
        brick['body'] = self._generate_body_brick_instance(module)
        
        code = template.process(brick)
        self._add_code_for_hook('statement_enum', code)


class ParserIdentifierGenerator(CodeGenerator):
    """Implements generator routines for producing
    code blocks responsible for definition of parser identifiers.
    
    Relevant template: parser_identifier.tpl"""
    
    def __init__(self, source_file=None):
        CodeGenerator.__init__(self, source_file)
        
    def _generate_body_brick_instance(self, module):
        # properties
        enumType = self.stmt_enum_prefix + module.function_name.upper()
        token = self.parser_token_prefix + module.function_name.upper()
        
        # generate
        brick_instance = {}
        brick_instance['<statement_enum_type>'] = enumType
        brick_instance['<statement_token>'] = token
        return brick_instance
    
    def generate(self, template, module):
        if template.name() != 'parser_identifier' and not module.valid():
            raise NameError('Cannot handle integration hook "%s"' % template.name())
        
        brick = {}
        brick['body'] = self._generate_body_brick_instance(module)
        
        code = template.process(brick)
        self._add_code_for_hook('parser_identifier', code)
        
        
class ParserTokenGenerator(CodeGenerator):
    """Implements generator routines for producing
    code blocks responsible for parser tokens.
    
    Relevant template: parser_token.tpl"""
    
    def __init__(self, source_file=None):
        CodeGenerator.__init__(self, source_file)
        
    def _generate_body_brick_instance(self, module):
        # properties
        token = self.parser_token_prefix + module.function_name.upper()
        
        # generate
        brick_instance = {}
        brick_instance['<statement_token>'] = token
        return brick_instance
    
    def generate(self, template, module):
        if template.name() != 'parser_token' and not module.valid():
            raise NameError('Cannot handle integration hook "%s"' % template.name())
        
        brick = {}
        brick['body'] = self._generate_body_brick_instance(module)
        
        code = template.process(brick)
        self._add_code_for_hook('parser_token', code)


class ScannerKeywordGenerator(CodeGenerator):
    """Implements generator routines for producing
    code blocks responsible for scanner keywords.
    
    Relevant template: scanner_keyword.tpl"""
    
    def __init__(self, source_file=None):
        CodeGenerator.__init__(self, source_file)
        
    def _generate_body_brick_instance(self, module):
        # properties
        token = self.parser_token_prefix + module.function_name.upper()
        
        # generate
        brick_instance = {}
        brick_instance['<statement_name>'] = module.function_name
        brick_instance['<statement_token>'] = token
        return brick_instance
    
    def generate(self, template, module):
        if template.name() != 'scanner_keyword' and not module.valid():
            raise NameError('Cannot handle integration hook "%s"' % template.name())
        
        brick = {}
        brick['body'] = self._generate_body_brick_instance(module)
        
        code = template.process(brick)
        self._add_code_for_hook('scanner_keyword', code)


class ModuleIncludeGenerator(CodeGenerator):
    """Implements generator routines for producing
    code blocks responsible for module source file includes.
    
    Relevant template: module_include.tpl"""
    
    def __init__(self, source_file=None):
        CodeGenerator.__init__(self, source_file)
        
    def _generate_body_brick_instance(self, module):
        # properties
        #token = self.parser_token_prefix + module.function_name.upper()
        
        # generate
        brick_instance = {}
        brick_instance['<module_file_name>'] = module.file_name()
        return brick_instance
    
    def generate(self, template, module):
        if template.name() != 'module_include' and not module.valid():
            raise NameError('Cannot handle integration hook "%s"' % template.name())
        
        brick = {}
        brick['body'] = self._generate_body_brick_instance(module)
        
        code = template.process(brick)
        self._add_code_for_hook('module_include', code)
    
