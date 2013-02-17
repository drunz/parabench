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


class Module:
    """Parses the module source file and analyzes certain parameters
    which are important for integrating the kernel as a module into Parabench.
    
    The module's function header will be analyzed for the following parameters:
    * Return Value
      - gboolean
      - IOStatus
    * Function Name
    * Parameter List
      - data type: gchar*, gint, glong
      - name: any
      - tags: _IN, _OUT, _INOUT"""
    
    return_types = ['IOStatus',] #['void', 'gboolean', 'IOStatus'] # allowed return types for modules
    
    def __init__(self, file_name, module_type='command'):
        self._file_name = file_name;
        self._is_valid = False
        self._parse()
    
    def _parse(self):
        module_file = open(self._file_name, 'r')
        
        pattern = r'^[ ]*(?P<return_type>' +'|'.join(self.return_types) +')[ ]+(?P<function_name>[A-Za-z0-9_]*)[ ]*[(](?P<parameters>[A-Za-z0-9\* ,_]*)[)]'
        prog = re.compile(pattern)
        
        for line in module_file:
            m = prog.match(line)
            
            if m:
                self.return_type   = m.group('return_type')
                self.function_name = m.group('function_name')
                
                self.parameter_list = []
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
                        
                    self.parameter_list.append((token[0].strip(), token[1].strip()))
                
                #print self.parameter_list
                self._is_valid = True
                break
            
        module_file.close()
        
    def is_valid(self):
        return self._is_valid
    
    def get_file_name(self):
        return self._file_name.rsplit('/', 1)[-1]
        
    def print_parameters(self):
        print 'Parameters of module \"' + self.get_file_name() + '\"'
        print '* Return Type   =', self.return_type
        print '* Function Name =', self.function_name
        print '* Parameters    =', self.parameter_list
