const fs = require('fs');
const file = 'src/App.tsx';
let content = fs.readFileSync(file, 'utf8');

content = content.replace(
  "                <div className=\"user-list\">\n                  {[...users].sort((a, b) => a.userName.localeCompare(b.userName)).map(u => {",
  "                <div className=\"user-list\">\n                  {[...users].sort((a, b) => {\n                    const aIgnored = ignoredUsers.includes(a.userId);\n                    const bIgnored = ignoredUsers.includes(b.userId);\n                    if (aIgnored !== bIgnored) return aIgnored ? 1 : -1;\n                    return a.userName.localeCompare(b.userName);\n                  }).map(u => {"
);

fs.writeFileSync(file, content);
console.log('Patched App.tsx for sort');
