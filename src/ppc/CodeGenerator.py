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
    _hookMap = {}
    
    def __init__(self, sourceFile):
        if sourceFile:
            self._sourceFile = sourceFile
            self._targetFile = self._genFileSuffix(sourceFile)
            self.pureGenerator = False
        else:
            self.pureGenerator = True
        
        hookFormat = r'^[ ]*[\t]*(/\*)[ ]*(!\[ModuleHook\])[ ]+(?P<hook_name>[A-Za-z0-9_]+)[ ]*(\*/)'
        self._hookProg = re.compile(hookFormat)
    
    
    def _genFileSuffix(self, fileName):
        return 'gen/'+fileName
        if '/' in fileName:
            parts = fileName.rsplit('/', 1)
            return parts[0]+'/gen/'+parts[1]
        else:
            return fileName + '.gen'
        
    
    def _add(self, hook, code):
        if hook in self._hookMap:
            self._hookMap[hook] += code
        else:
            self._hookMap[hook]  = code
        #print 'OK...'
    
    
    def write(self):
        if not self.pureGenerator:
            source = open(self._sourceFile, 'r')
            target = open(self._targetFile, 'w')
            
            # Iterate over source lines and insert code at hook if found
            for line in source:
                # Write the code to the found hook
                m = self._hookProg.match(line)
                if m:
                    hookName = m.group('hook_name')
                    target.write('  /* BEGIN - Automatically generated code. Do not change! */\n')
                    target.write(self._hookMap[hookName])
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
    stmtEnumPrefix = 'STMT_MOD_'
    parserTokenPrefix = 'TMOD_'
    paramFetchMap = {'gchar*':'param_string_get', 'glong':'param_int_get'}

    """
    Passing None as sourceFile will create a pure generator,
    which is not able to write code. If a source file name is given,
    the generator searches for hooks in this file and replaces them
    with already generated code.
    """
    def __init__(self, sourceFile = None):
        SourceWriter.__init__(self, sourceFile)
    
    
    def printCode(self):
        for key, value in self._hookMap.iteritems():
            print '[%s]\n' % key
            print value
            print '\n\n'
    
    
    def generate(self, template, module):
        raise TypeError('Abstract method called')


            
class StatementExecGenerator(CodeGenerator):
    def __init__(self, sourceFile = None):
        CodeGenerator.__init__(self, sourceFile)
        
        
    def _generateBodyBrickInstance(self, module):
        # properties
        enumType = self.stmtEnumPrefix + module.functionName.upper()
        numParam = len(module.parameterList)
        
        # generate
        brickInstance = {}
        brickInstance['<statement_enum_type>'] = enumType
        brickInstance['<num_param>'] = str(numParam)
        return brickInstance
    
    
    def _generateFetchBrickInstances(self, module):
        # properties
        numParam = len(module.parameterList)
        parameterList = module.parameterList
        
        # generate
        instanceList = []
        brickInstance = {}
        for i in range(numParam):
            paramType = parameterList[i][0]
            paramName = parameterList[i][1]
            brickInstance = {"<param_type>":paramType, 
                             "<param_name>":paramName, 
                             "<param_fetch_call>":self.paramFetchMap[paramType], 
                             "<param_id>":str(i)}
            instanceList.append(brickInstance)
        return instanceList
    
    
    def _generateAssertBrickInstances(self, module):
        # properties
        numParam = len(module.parameterList)
        
        # generate
        instanceList = []
        brickInstance = {'<num_param>':str(numParam)}
        
        instanceList.append(brickInstance)
        return instanceList
    
    
    def _generateCallBrickInstances(self, module):
        # properties
        parameterList = ', '.join(map(lambda x: x[1], module.parameterList))
        enumType = self.stmtEnumPrefix + module.functionName.upper()
        
        # generate
        instanceList = []
        brickInstance = {'<io_func_call>':module.functionName,
                         '<io_func_param_list>':parameterList,
                         '<statement_enum_type>':enumType}
        
        instanceList.append(brickInstance)
        return instanceList
    
    
    def generate(self, template, module):
        if template.getName() != 'statement_exec' and not module.isValid():
            raise NameError('Cannot handle integration hook "%s"' % template.getName())
        
        brick = {}
        brick['body']             = self._generateBodyBrickInstance(module)
        brick['parameter_fetch']  = self._generateFetchBrickInstances(module)
        brick['parameter_assert'] = self._generateAssertBrickInstances(module)
        brick['module_call']      = self._generateCallBrickInstances(module)
        
        code = template.process(brick)
        self._add('statement_exec', code)



class StatementEnumGenerator(CodeGenerator):
    def __init__(self, sourceFile = None):
        CodeGenerator.__init__(self, sourceFile)
        
        
    def _generateBodyBrickInstance(self, module):
        # properties
        enumType = self.stmtEnumPrefix + module.functionName.upper()
        
        # generate
        brickInstance = {}
        brickInstance['<statement_enum_type>'] = enumType
        return brickInstance
    
    
    def generate(self, template, module):
        if template.getName() != 'statement_enum' and not module.isValid():
            raise NameError('Cannot handle integration hook "%s"' % template.getName())
        
        brick = {}
        brick['body'] = self._generateBodyBrickInstance(module)
        
        code = template.process(brick)
        self._add('statement_enum', code)



class ParserIdentifierGenerator(CodeGenerator):
    def __init__(self, sourceFile = None):
        CodeGenerator.__init__(self, sourceFile)
        
        
    def _generateBodyBrickInstance(self, module):
        # properties
        enumType = self.stmtEnumPrefix + module.functionName.upper()
        token = self.parserTokenPrefix + module.functionName.upper()
        
        # generate
        brickInstance = {}
        brickInstance['<statement_enum_type>'] = enumType
        brickInstance['<statement_token>'] = token
        return brickInstance
    
    
    def generate(self, template, module):
        if template.getName() != 'parser_identifier' and not module.isValid():
            raise NameError('Cannot handle integration hook "%s"' % template.getName())
        
        brick = {}
        brick['body'] = self._generateBodyBrickInstance(module)
        
        code = template.process(brick)
        self._add('parser_identifier', code)
        
        
        
class ParserTokenGenerator(CodeGenerator):
    def __init__(self, sourceFile = None):
        CodeGenerator.__init__(self, sourceFile)
        
        
    def _generateBodyBrickInstance(self, module):
        # properties
        token = self.parserTokenPrefix + module.functionName.upper()
        
        # generate
        brickInstance = {}
        brickInstance['<statement_token>'] = token
        return brickInstance
    
    
    def generate(self, template, module):
        if template.getName() != 'parser_token' and not module.isValid():
            raise NameError('Cannot handle integration hook "%s"' % template.getName())
        
        brick = {}
        brick['body'] = self._generateBodyBrickInstance(module)
        
        code = template.process(brick)
        self._add('parser_token', code)



class ScannerKeywordGenerator(CodeGenerator):
    def __init__(self, sourceFile = None):
        CodeGenerator.__init__(self, sourceFile)
        
        
    def _generateBodyBrickInstance(self, module):
        # properties
        token = self.parserTokenPrefix + module.functionName.upper()
        
        # generate
        brickInstance = {}
        brickInstance['<statement_name>'] = module.functionName
        brickInstance['<statement_token>'] = token
        return brickInstance
    
    
    def generate(self, template, module):
        if template.getName() != 'scanner_keyword' and not module.isValid():
            raise NameError('Cannot handle integration hook "%s"' % template.getName())
        
        brick = {}
        brick['body'] = self._generateBodyBrickInstance(module)
        
        code = template.process(brick)
        self._add('scanner_keyword', code)
        


class ModuleIncludeGenerator(CodeGenerator):
    def __init__(self, sourceFile = None):
        CodeGenerator.__init__(self, sourceFile)
        
        
    def _generateBodyBrickInstance(self, module):
        # properties
        token = self.parserTokenPrefix + module.functionName.upper()
        
        # generate
        brickInstance = {}
        brickInstance['<module_file_name>'] = module.getFileName()
        return brickInstance
    
    
    def generate(self, template, module):
        if template.getName() != 'module_include' and not module.isValid():
            raise NameError('Cannot handle integration hook "%s"' % template.getName())
        
        brick = {}
        brick['body'] = self._generateBodyBrickInstance(module)
        
        code = template.process(brick)
        self._add('module_include', code)
    
