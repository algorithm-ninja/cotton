/*

Copyright (2016) - Dario Ostuni <another.code.996@gmail.com>
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

const _ = require('lodash');
const fse = require('fs-extra');
const childProcess = require('child_process');
const path = require('path');
const should = require('should/as-function');

/**
 * Cotton node interface
 * =====================
 *
 * Cotton sandbox wrapper for node.
 */

module.exports = class CottonSandbox {
  /**
   * Creates a new Cotton sandbox to work with, the function teardown() must be
   * called to destroy it at the end.
   *
   * @param {!String} boxType The type of box you need (e.g. DummyUnixSandbox).
   * @constructor
   */
  constructor(boxType) {
    should(boxType).be.String();

    const sandboxId = parseInt(this._execute(['create', boxType]));
    should(sandboxId).be.Number('Error while creating the box');

    this._boxType = boxType;
    this._sandboxId = sandboxId;
    this._rootDir = path.join('/tmp', 'box_' + this._sandboxId, 'file_root');
  }

  /**
   * Deletes the sandbox. Functions of this object shouldn't be called after
   * this.
   */
  destroy() {
    this._executeOnSandbox(['destroy']);
  }

  /**
   * Returns the absolute path to the root directory of the sandbox.
   *
   * @return {string} the path to the sandbox directory.
   */
  getRoot() {
    return this._rootDir;
  }

  /**
   * Redirects stdin from a file (it must be outside of the sandbox'd
   * directory).
   *
   * @param {!string} filename the name of the file.
   * @return {CottonSandbox} the current object for chaining.
   */
  stdin(filename) {
    return this._redirect('stdin', filename);
  }

  /**
   * Redirects stdout to a file (it must be outside of the sandbox'd directory).
   *
   * @param {string} filename the name of the file.
   * @return {CottonSandbox} the current object for chaining.
   */
  stdout(filename) {
    return this._redirect('stdout', filename);
  }

  /**
   * Redirects stderr to a file (it must be outside of the sandbox'd directory).
   *
   * @param {string} filename the name of the file.
   * @return {CottonSandbox} the current object.
   */
  stderr(filename) {
    return this._redirect('stderr', filename);
  }

  /**
   * Sets the CPU time limit for a command execution.
   *
   * @todo Delete this when `timeLimit()` will accept keyword arguments.
   * @warning This does not limit the real time used by a process. For
   *     instance, using usleep() may leave the process hanging for a long
   *     time.
   * @param {number} time the CPU time limit in seconds.
   * @return {CottonSandbox} the current object for chaining.
   */
  cpuTimeLimit(time) {
    should(time)
        .be.a.Number()
        .and.not.be.Infinity()
        .and.be.above(0);
    this._executeOnSandbox(['cpu-limit', time]);
    return this;
  }

  /**
   * Sets the wall time limit for a command execution.
   *
   * @todo Delete this when `timeLimit()` will accept keyword arguments.
   * @param {number} time the wall time limit in seconds.
   * @return {CottonSandbox} the current object for chaining.
   */
  wallTimeLimit(time) {
    should(time)
        .be.a.Number()
        .and.not.be.Infinity()
        .and.be.above(0);
    this._executeOnSandbox(['wall-limit', time]);
    return this;
  }

  /**
   * Sets the time limit (both wall time and CPU time) for a command
   * execution. The wall time is assumed to be `time + 1`, while the
   * CPU time is set to `time`.
   *
   * @todo Use keyword arguments `cpuTime` and `wallTime`.
   * @param {number} time the time limit in seconds.
   * @return {CottonSandbox} the current object for chaining.
   */
  timeLimit(time) {
    return this.cpuTimeLime(time).wallTimeLimit(time + 1);
  }

  /**
   * Sets the memory limit for a command execution.
   *
   * @param {number} memory the memory limit in MiB.
   * @return {CottonSandbox} the current object for chaining.
   */
  memoryLimit(memory) {
    should(memory)
        .be.a.Number()
        .and.not.be.Infinity()
        .and.be.above(0);
    this._executeOnSandbox(['memory-limit', memory]);
    return this;
  }

  /**
   * Sets the disk limit for a command execution.
   *
   * @param {number} memory the memory limit in MiB.
   * @return {CottonSandbox} the current object for chaining.
   */
  diskLimit(memory) {
    should(memory)
        .be.a.Number()
        .and.not.be.Infinity()
        .and.be.above(0);
    this._executeOnSandbox(['disk-limit', memory]);
    return this;
  }

  /**
   * Sets the executable bit in a file inside of the sandbox'd directory.
   *
   * @param {string} filename the file name.
   * @return {CottonSandbox} the current object for chaining.
   */
  executable(filename) {
    should(filename).be.a.String();
    should(filename.length).be.above(0);
    fse.chmodSync(path.join(this._rootDir, filename), 0o775);
    return this;
  }

  /**
   * Runs a command. The command will be interpreted as a relative path inside
   * the sandbox'd directory.
   *
   * @param {!string} command the command.
   * @param {?Array} args the arguments.
   * @return {Object} the execution status. It is composed of the following
   *                  fields:
   *                  - returnCode
   *                  - cpuTime
   *                  - wallTime
   *                  - memoryPeak (MRSS)
   *                  - signal
   */
  run(command, args) {
    should(command).be.String();
    if (_.isNil(args)) {
      args = [];
    } else {
      args = _.castArray(args);
    }

    args.unshift('run', command);
    this._executeOnSandbox(args);

    const ret = {};
    ret.returnCode = this.returnCode();
    ret.cpuTime = this.cpuTime();
    ret.wallTime = this.wallTime();
    ret.memory = this.memoryPeak();
    ret.signal = this.signal();

    return ret;
  }

  /**
   * Retrieves the return code of the last command execution.
   *
   * @return {Number}
   */
  returnCode() {
    return parseInt(this._executeOnSandbox(['return-code']));
  }

  /**
   * Retrieves the cpu time consumed by the last command execution (us).
   *
   * @return {Number}
   */
  cpuTime() {
    return parseInt(this._executeOnSandbox(['running-time']));
  }

  /**
   * Retrieves the wall time consumed by the last command execution (us).
   *
   * @return {Number}
   */
  wallTime() {
    return parseInt(this._executeOnSandbox(['wall-time']));
  }

  /**
   * Retrieves the maximum resident set size (MRSS) allocated by the last
   * command execution (bytes).
   *
   * @return {Number}
   */
  memoryPeak() {
    return parseInt(this._executeOnSandbox(['memory-usage']));
  }

  /**
   * Retrieves the signal that killed the last command execution.
   *
   * @return {Number}
   */
  signal() {
    return parseInt(this._executeOnSandbox(['signal']));
  }

  /**
   * Reads the content of a file.
   *
   * @param {string} filePath the file path relative to the root.
   * @return {string} File content.
   */
  readFile(filePath) {
    return fse.readFileSync(path.join(this._rootDir, filePath))
        .toString('utf-8');
  }

  /**
   * Removes a file inside the sandbox'd directory.
   *
   * @param {string} filename the file name.
   * @return {CottonSandbox} the current object.
   */
  removeFile(filename) {
    should(filename).be.String();
    should(filename.length).be.above(0);

    fse.unlinkSync(path.join(this._rootDir, filename));
    return this;
  }

  /**
   * Creates a symlink to a file.
   *
   * @param {!string} filePath File path.
   * @param {!string} destination Link path, relative to the box root.
   * @return {CottonSandbox} the current object for chaining
   */
  symlink(filePath, destination) {
    should(filePath).be.String();
    should(destination).be.String();

    fse.symlinkSync(filePath, path.join(this._rootDir, destination));
    return this;
  }

  /**
   * Enable a path in the sandbox (read-only).
   *
   * @param {!string} internalPath
   * @param {!string} path
   */
  mountRO(internalPath, path) {
    should(internalPath).be.String();
    should(path).be.String();

    this._executeOnSandbox(['mount', internalPath, path]);
  }

  /**
   * Enable a path in the sandbox (read-write).
   *
   * @param {!string} intrnalPath
   * @param {!string} path
   */
  mountRW(internalPath, path) {
    should(internalPath).be.String();
    should(path).be.String();

    this._executeOnSandbox(['mount', '--rw', internalPath, path]);
  }


  /**
   * Runs a command calling `cotton -j ...`.
   *
   * @private
   * @param {Array} args the command to run with its arguments.
   * @return {Object} the JSON returned by Cotton.
   */
  _execute(args) {
    should(args).be.an.Array();

    // Force json output.
    args.unshift('-j');

    // Spawn the command.
    const rawOutput = childProcess.spawnSync('cotton', args).stdout
        .toString('utf-8');
    const outcome = JSON.parse(rawOutput);

    console.log(args);
    console.log(outcome);

    if (!_.isEmpty(outcome.errors)) {
      throw new Error(JSON.stringify(outcome.errors));
    }
    return outcome.result;
  }

  /**
   * Runs a command calling `cotton -j -b {sandboxId} ...`.
   *
   * @private
   * @param {Array} args the arguments.
   * @return {Object} the JSON returned by Cotton.
   */
  _executeOnSandbox(args) {
    args.unshift('-b', this._sandboxId);
    return this._execute(args);
  }

  /**
   * This function sets the streams redirection in the sandbox.
   *
   * @private
   * @param {!string} stream the name of the stream (stdin, stdout or stderr).
   * @param {!string} filname the file name of the redirected stream.
   * @return {CottonSandbox} the current object.
   */
  _redirect(stream, filename) {
    should(stream).be.equalOneOf(['stdin', 'stdout', 'stderr']);
    should(filename).be.String();
    should(filename.length).be.above(0);

    this._executeOnSandbox(['redirect', stream, filename]);
    return this;
  }
};
