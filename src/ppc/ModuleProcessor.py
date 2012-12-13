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
Parses the module source file and analyzes certain parameters
which are important for integrating the kernel as a module into Parabench.

The module's function header will be analyzed for the following parameters:
* Return Value
  - gboolean
  - IOStatus
* Function Name
* Parameter List
  - data type: gchar*, gint, glong
  - name: any
  - tags: _IN, _OUT, _INOUT
"""

import re


class Module:
    returnTypes = ['IOStatus',] #['void', 'gboolean', 'IOStatus'] # allowed return types for modules
    
    def __init__(self, fileName, moduleType='command'):
        self._fileName = fileName;
        self._isValid = False
        self._parse()
    
    def _parse(self):
        moduleFile = open(self._fileName, 'r')
        
        pattern = r'^[ ]*(?P<return_type>' +'|'.join(self.returnTypes) +')[ ]+(?P<function_name>[A-Za-z0-9_]*)[ ]*[(](?P<parameters>[A-Za-z0-9\* ,_]*)[)]'
        prog = re.compile(pattern)
        
        for line in moduleFile:
            m = prog.match(line)
            
            if m:
                self.returnType    = m.group('return_type')
                self.functionName  = m.group('function_name')
                
                self.parameterList = []
                for param in m.group('parameters').split(','):
                    # token[0] = type ('gchar*', 'glong')
                    # token[1] = name ('fname', 'offset', ...)
                    token = param.rsplit(None, 1)
                    # remove const modifier
                    token[0] = token[0].replace('const', '').strip()
                    
                    # when user places the star operator to the qualifier name, we move it to the type
                    # gchar *str --> gchar* str, gchar **str --> gchar** str, ...
                    while token[1].startswith('*'):
                        token[1]  = token[1][1:]
                        token[0] += '*'
                        
                    self.parameterList.append((token[0].strip(), token[1].strip()))
                
                #print self.parameterList
                self._isValid = True
                break
            
        moduleFile.close()
        
    def isValid(self):
        return self._isValid
    
    def getFileName(self):
        return self._fileName.rsplit('/', 1)[-1]
        
    def printParameters(self):
        print 'Parameters of module \"' + self.getFileName() + '\"'
        print '* Return Type   =', self.returnType
        print '* Function Name =', self.functionName
        print '* Parameters    =', self.parameterList
