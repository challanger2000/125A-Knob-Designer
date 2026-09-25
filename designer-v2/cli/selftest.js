import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

const root = path.resolve(process.cwd());
const cli = path.join(root, 'designer-v2/cli/125a-gui.js');
const sample = path.join(root, 'designer-v2/examples/knob-dark-metal.125agui.json');
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), '125a-gui-'));
const work = path.join(tmp, 'sample.125agui.json');
fs.copyFileSync(sample, work);

function run(args) {
  return execFileSync(process.execPath, [cli, ...args], { encoding: 'utf8' });
}

if (!run(['validate', work]).includes('PASS:')) throw new Error('validate failed');
run(['set', work, 'output.frameCount', '64', '--in-place']);
let p = JSON.parse(fs.readFileSync(work, 'utf8'));
if (p.output.frameCount !== 64) throw new Error('set failed');
run(['preset', work, 'industrial-steel', '--in-place']);
p = JSON.parse(fs.readFileSync(work, 'utf8'));
if (p.design.material !== 'steel' || p.design.shape !== 'stepped') throw new Error('preset failed');
if (!run(['validate', work]).includes('PASS:')) throw new Error('post-edit validate failed');

console.log('125A GUI Designer v0.2.0 foundation self-test: PASS');
