/*

Copyright (2016) - Dario Ostuni <another.code.996@gmail.com>

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

const fse = require('fs-extra');
const childProcess = require('child_process');
const path = require('path');
const should = require('should');

/**
 * Cotton Node.js interface
 * ========================
 *
 * This class exposes the Cotton sandbox functions to Node.js
 *
 */

module.exports = class CottonSandbox {
  /**
   * This function runs a command within the sandbox
   *
   * @private
   * @param {Array} the command to run with its arguments
   * @return {Object} the JSON returned by Cotton
   */
  _execute(args) {
      should(args).be.an.Array();
      args.unshift('-j');
      let status = childProcess.spawnSync('cotton', args);
      should(status.status).not.be.ok();
      let retJSON = JSON.parse(status.stdout);
      should(retJSON.result).be.ok();
      return retJSON;
  }

  /**
   * Creates a new Cotton sandbox to work with, the function teardown() must be
   * called to destroy it at the end
   *
   * @constructor
   */
  constructor() {
    let sbNum = parseInt(this._execute(['create', 'DummyUnixSandbox']).result);
    should(sbNum).be.ok();
    this._sandboxNumber = sbNum;
    this._rootDir = '/tmp/box_' + this._sandboxNumber + '/file_root/';
  }

  /**
   * Deletes the sandbox. Functions of this object shouldn't be called after
   * this
   */
  teardown() {
    this._execute(['delete', this._sandboxNumber]);
  }

  /**
   * Adds a file to the sandbox which will be outside of sandbox'd directory.
   * Use this function to add an input file
   *
   * @param {string} the path to the file to add
   * @param {string} the name the file will have inside the sandbox
   * @return {CottonSandbox} the current object
   */
  addExtern(filename, stream) {
    return this.add(filename, stream, '/tmp/box_' + this._sandboxNumber + '/');
  }

  /**
   * Adds a file to the sandbox which will be inside of sandbox'd directory.
   * Use this to add a source file or a result checker
   *
   * @param {string} the path to the file to add
   * @param {string} the name the file will have inside the sandbox
   * @param {?string} don't use this unless you know what you're doing
   * @return {CottonSandbox} the current object
   */
  add(filename, newFilename, internalPath) {
    if(!internalPath) {
        internalPath = this._rootDir;
    }
    should(filename).be.a.String();
    should(fse.statSync(filename).isFile()).be.true();
    if (newFilename) {
      should(newFilename).be.a.String();
      should(newFilename.length).be.above(0);
    }
    try {
      newFilename = newFilename || path.basename(filename);
      newFilename = path.join(internalPath, newFilename);
      fse.copySync(filename, newFilename);
    } catch (err) {
      console.error(err);
    }

    return this;
  }

  /**
   * This function sets the streams redirection in the sandbox
   *
   * @private
   * @param {string} the name of the stream (stdin, stdout or stderr)
   * @param {string} the file name of the redirected stream
   * @return {CottonSandbox} the current object
   */
  _redirect(stream, filename) {
    should(stream).be.equalOneOf(['stdin', 'stdout', 'stderr']);
    should(filename).be.a.String();
    should(filename.length).be.above(0);
    this._execute(['set-redirect', this._sandboxNumber, stream, filename]);
    return this;
  }

  /**
   * Redirects stdin from a file (it must be outside of the sandbox'd directory)
   *
   * @param {string} the name of the file
   * @return {CottonSandbox} the current object
   */
  stdin(filename) {
    return this._redirect('stdin', filename);
  }

  /**
   * Redirects stdout to a file (it must be outside of the sandbox'd directory)
   *
   * @param {string} the name of the file
   * @return {CottonSandbox} the current object
   */
  stdout(filename) {
    return this._redirect('stdout', filename);
  }

  /**
   * Redirects stderr to a file (it must be outside of the sandbox'd directory)
   *
   * @param {string} the name of the file
   * @return {CottonSandbox} the current object
   */
  stderr(filename) {
    return this._redirect('stderr', filename);
  }

  /**
   * Sets the time limit for a command execution
   *
   * @param {number} the time limit in seconds
   * @return {CottonSandbox} the current object
   */
  timeLimit(time) {
    should(time)
        .be.a.Number()
        .and.not.be.Infinity()
        .and.be.above(0);
    this._execute(['set-limit', this._sandboxNumber, 'time', time]);
    return this;
  }

  /**
   * Sets the memory limit for a command execution
   *
   * @param {number} the memory limit in MiB
   * @return {CottonSandbox} the current object
   */
  memoryLimit(memory) {
    should(memory)
        .be.a.Number()
        .and.not.be.Infinity()
        .and.be.above(0);
    this._execute(['set-limit', this._sandboxNumber, 'memory', memory]);
    return this;
  }

  /**
   * Sets the executable bit in a file inside of the sandbox'd directory
   *
   * @param {string} the file name
   * @return {CottonSandbox} the current object
   */
  executable(filename) {
    should(filename).be.a.String();
    should(filename.length).be.above(0);
    fse.chmodSync(path.join(this._rootDir, filename), 0o775);
    return this;
  }

  /**
   * Runs a command. The command will be interpreted as an absolute path
   *
   * @param {string} the command
   * @param {Array} the arguments
   * @return {Object} the execution status
   */
  run(command, args) {
    should(command).be.String();
    should(args).be.Array();
    let cmdPath = childProcess.spawnSync('which', [command]).stdout;
    fse.symlinkSync(cmdPath, this._rootDir + command);
    let retValue = this.runRelative(command, args);
    fse.unlinkSync(this._rootDir + command);
    return retValue;
  }

  /**
   * An helper function to get the status of an execution
   *
   * @private
   * @param {string} the type of information needed
   * @return {Object} the JSON from Cotton
   */
  _getStatus(type) {
    should(type).be.String();
    return this._execute(['get', this._sandboxNumber, type]).result;
  }

  /**
   * Runs a command. The command will be interpreted as a relative path inside
   * the sandbox'd directory
   *
   * @param {string} the command
   * @param {Array} the arguments
   * @return {Object} the execution status
   */
  runRelative(command, args) {
    should(command).be.String();
    should(args).be.Array();
    args.unshift('run', this._sandboxNumber, command);
    this._execute(args);
    let ret = {};
    ret.status = this._getStatus('return-code');
    ret.time = this._getStatus('running-time') / 1000000.0;
    ret.memory = this._getStatus('memory-usage') / 1024.0;
    return ret;
  }

  /**
   * Removes a file inside the sandbox'd directory
   *
   * @param {string} the file name
   * @return {CottonSandbox} the current object
   */
  remove(filename) {
    should(filename).be.String();
    should(filename.length).be.above(0);
    fse.unlinkSync(this._rootDir + filename);
    return this;
  }

  /**
   * Moves a stream redirection file inside the sandbox'd directory
   *
   * @param {string} the file name
   * @return {CottonSandbox} the current object
   */
  importStream(filename) {
    should(filename).be.String();
    should(filename.length).be.above(0);
    fse.copySync('/tmp/box_' + this._sandboxNumber + '/' + filename,
                 this._rootDir + filename);
    return this;
  }
};
