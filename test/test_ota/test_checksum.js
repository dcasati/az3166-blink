const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const fs = require('node:fs');
const vm = require('node:vm');

const page = fs.readFileSync('include/OtaPage.h', 'utf8');
const script = page.split('<script>')[1].split("const token=")[0];
const context = vm.createContext({});
vm.runInContext(script, context);
for (const length of [0, 3, 55, 56, 63, 64, 65, 512, 395400, 622592, 999424]) {
  const bytes = crypto.randomBytes(length);
  const input = bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength);
  const actual = Buffer.from(context.sha256Fallback(input)).toString('hex');
  assert.equal(actual, crypto.createHash('sha256').update(bytes).digest('hex'));
}
console.log('Browser HTTP SHA-256 fallback checksums match Node crypto');
