#!/usr/bin/env python
#
# The MIT License (MIT)
# Copyright (c) 2020 stnuessl
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.
#

import argparse
import configparser
import os
import sys
import json
import glob
import jinja2
import datetime

HTML_TEMPLATE_REPORT = (
    '<table>\n'
    '   <thead>\n'
    '       <tr>\n'
    '           <th>Test</th>\n'
    '           <th>Result</th>\n'
    '       </tr>\n'
    '   </thead>\n'
    '   <tbody>\n'
    '   {%- for result in results %}\n'
    '       <tr>\n'
    '           <td>{{ result.test.name }}</td>\n'
    '           {%- if result.result %}\n'
    '           <td>PASS</td>\n'
    '           {%- else %}\n'
    '           <td>FAIL</td>\n'
    '           {%- endif %}\n'
    '       </tr>\n'
    '   {%- endfor %}\n'
    '   </tbody>\n'
    '</table>\n'
)


class GdbCheckConfig():
    CONFIG_NAME = 'gdb-check'

    def __load(self, path):
        if not os.path.isfile(path):
            raise Exception('\'path\' must reference a file')
        
        self.config = configparser.ConfigParser(
            default_section='gdb-check',
            allow_no_value=True
        )

        self.config.read(path)
        self.path = path

    def __init__(self, path=None):
        if path != None and os.path.isfile(path):
            self.__load(path)
            return

        if path == None:
            path = os.getcwd()

        while path != '/':
            prefixes = ['', '.']

            for x in prefixes:
                tmp = '{}/{}{}'.format(path, x, self.CONFIG_NAME)

                if os.path.isfile(tmp):
                    self.__load(tmp)
                    return

                path = os.path.split(path)[0]

        print('gdb-check: failed to detect configuration file', file=sys.stderr)
        sys.exit(1)

    def get_program(self):
        return self.config['gdb-check']['Program']

    def get_test_cases(self):
        cwd = os.getcwd();
        dname = os.path.dirname(self.path)
        path = self.config['gdb-check']['TestSpecDirectory']
        path = [os.path.join(cwd, dname, x) for x in path.split() if x != '']

        tests = []

        for x in path:
            files = glob.glob('{}/*.json'.format(x), recursive=True)

            for x in files:
                items = json.load(open(x, 'r'))

                tests.extend(items)

        # Sanitize the test cases so that they are easier to process later on
        for i, test in enumerate(tests):

            # The user may omit some data fields
            pairs = [
                ('description', ''),
                ('enable', True),
                ('cpu', ''),
                ('process', ''),
                ('thread', ''),
                ('comment', ''),
                ('tags', ''),
                ('arguments', ''),
                ('initialization', []),
                ('stubs', []),
                ('preconditions', []),
                ('stimulation', []),
                ('expectation', []),
                ('postconditions', []),
                ('finalization', []),
            ]

            [test.setdefault(*x) for x in pairs]

            #
            # Sanitize all list elements:
            # The user is able the comment out single elements
            # by prepending a "#" to them.
            #
            for k in [k for k, v in pairs if isinstance(v, list)]:
                test[k] = [x for x in test[k]
                           if not isinstance(x, str)
                           or not x.strip().startswith('#')]

            if isinstance(test['arguments'], list):
                test['arguments'] = ', '.join((str(x) for x in test['arguments']))


#            for x in test['stubs']:
#                x['return'] = x.get('return', '')
#                x['postactions'] = x.get('postactions', [])
                
           
        return tests


class GdbCheck():
    def read_config(self, path):
        self.config = GdbCheckConfig(path)

    def generate_script(self):
        context = {
            'date' : datetime.date.today().strftime('%Y-%m-%d'),
            'python_gdb_script' : 'gdb/util.py',
            'logging_redirect_on' : 'on',
            'logging_on' : 'off',
            'binary' : self.config.get_program(),
            'tests' : self.config.get_test_cases(),
            'html_template_report' : HTML_TEMPLATE_REPORT,
        }
        template = jinja2.Template(open('template/gdb.jinja', 'r').read())
        script = template.render(context)

        print('{}'.format(script), file=sys.stdout)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'CONFIG', 
        nargs='?', 
        help='dbg-check configuration file'
    )

    args = parser.parse_args()
    
    app = GdbCheck()
    app.read_config(args.CONFIG)
    app.generate_script()

if __name__ == '__main__':
    main()

