import fs from 'node:fs';
const src = fs.readFileSync(new URL('../web/src/main.js', import.meta.url), 'utf8');

const ids = [...src.matchAll(/id="([^"]+)"/g)].map(m => m[1]);
const duplicates = ids.filter((id,i) => ids.indexOf(id) !== i);
if (duplicates.length) throw new Error('Duplicate DOM ids: ' + [...new Set(duplicates)].join(', '));

const referenced = [...src.matchAll(/\$\('([^']+)'\)/g)].map(m => m[1]);
const missing = [...new Set(referenced.filter(id => !ids.includes(id)))];
if (missing.length) throw new Error('DOM contract missing ids: ' + missing.join(', '));

console.log(`125A Simple Mode DOM contract: PASS (${ids.length} ids, ${new Set(referenced).size} referenced)`);
