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
from cStringIO import StringIO


class Template:
    """Generates blocks of code using template files.
    Brick maps contain placeholder -> value mappings that
    are used to generate code by replacing placeholders
    in templates with their corresponding values specified in
    the brick maps"""
    
    include_hook = '#include:'
    
    def __init__(self, template_name):
        self._template_name = template_name
        
        self._body_file = 'ppc/tpl/' + template_name + '.tpl'
        self._brick_files = glob.glob('ppc/tpl/' + template_name + '/*.tpl')
        
    def _process_brick(self, brick_map, brick_name):
        source_file = open('ppc/tpl/' + self._template_name + '/' +brick_name +'.tpl', 'r')
        code = StringIO()
        
        # For each brick instance for this brick name
        for brick_instance in brick_map[brick_name]:
            source_file.seek(0)
            for line in source_file:
                # Replace options
                for key, value in brick_instance.iteritems():
                    line = line.replace(key, value)
                code.write(line)
        
        source_file.close()
        return code.getvalue()
    
    def process(self, brick_map):
        #print self._body_file
        #print self._brick_files
        
        code = StringIO()
        body_file = open(self._body_file, 'r')
        
        for line in body_file:
            # If include found, include brick file and process to code
            if self.include_hook in line:
                code.write(self._process_brick(brick_map, line.split()[1]))
            # Else replace the options and add to code 
            else:
                for key, value in brick_map['body'].iteritems():
                    line = line.replace(key, value)
                code.write(line)
        
        body_file.close()
        return code.getvalue()
    
    def get_name(self):
        return self._template_name
    