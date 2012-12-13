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

import glob

class Template:
    includeHook = '#include:'
    
    
    def __init__(self, templateName):
        self._templateName = templateName
        
        self._bodyFile = 'ppc/tpl/' + templateName + '.tpl'
        self._brickFiles = glob.glob('ppc/tpl/' + templateName + '/*.tpl')
        
        
    def _processBrick(self, brickMap, brickName):
        file = open('ppc/tpl/' + self._templateName + '/' +brickName +'.tpl', 'r')
        code = ''
        
        # For each brick instance for this brick name
        for brickInstance in brickMap[brickName]:
            file.seek(0)
            for line in file:
                # Replace options
                for key, value in brickInstance.iteritems():
                    line = line.replace(key, value)
                code += line
        
        file.close()
        return code
        
    
    def process(self, brickMap):
        #print self._bodyFile
        #print self._brickFiles
        
        code = ''
        body = open(self._bodyFile, 'r')
        
        for line in body:
            # If include found, include brick file and process to code
            if self.includeHook in line:
                code += self._processBrick(brickMap, line.split()[1])
            # Else replace the options and add to code 
            else:
                for key, value in brickMap['body'].iteritems():
                    line = line.replace(key, value)
                code += line
        
        body.close()
        return code
    
    def getName(self):
        return self._templateName
    