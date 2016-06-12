/*

Copyright (2016) - Gabriele Farina <gabr.farina@gmail.com>
Copyright (2016) - William di Luigi <williamdiluigi@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

'use strict';

import test from 'ava';
import CottonSandbox from './CottonSandbox.js';

test('sandbox creation, type DummyUnixSandbox', t => {
  const sandbox = new CottonSandbox('DummyUnixSandbox');

  t.truthy(sandbox);
  t.truthy(sandbox._sandboxId);
  sandbox.destroy();
});

test('sandbox creation, type unknown', t => {
  t.throws(() => {new CottonSandbox('notvalid');});
});

test('ls works, type DummyUnixSandbox', t => {
  const sandbox = new CottonSandbox('DummyUnixSandbox');

  sandbox.symlink('/bin/ls', 'ls');
  const outcome = sandbox.stdout('ls_output').run('ls');
  const output = sandbox.readFile('ls_output');

  t.is(outcome.returnCode, 0);
  t.is(output, 'ls\nls_output\n');

  sandbox.destroy();
});
