import re

main = 'app/src/main/java/com/example/chat/MainActivity.kt'
with open(main, 'r', encoding='utf-8') as f:
    content = f.read()

# Change chat messages LazyColumn
content = re.sub(
    r'Win95Panel\(modifier = Modifier.weight\(7f\).fillMaxHeight\(\)\) \{\s*LazyColumn\(modifier = Modifier.fillMaxSize\(\).padding\(0.5.dp\)\)',
    r'Win95Panel(modifier = Modifier.weight(7f).fillMaxHeight()) {\n                    LazyColumn(modifier = Modifier.fillMaxSize().background(Color.White).padding(0.5.dp))',
    content
)

# Change user list LazyColumn and sort items
content = re.sub(
    r'Win95Panel\(modifier = Modifier.weight\(1f\).fillMaxWidth\(\)\) \{\s*LazyColumn\(modifier = Modifier.fillMaxSize\(\).padding\(4.dp\)\) \{\s*items\(users\)',
    r'Win95Panel(modifier = Modifier.weight(1f).fillMaxWidth()) {\n                        LazyColumn(modifier = Modifier.fillMaxSize().background(Color.White).padding(0.5.dp)) {\n                            items(users.sortedBy { it.userName })',
    content
)

with open(main, 'w', encoding='utf-8') as f:
    f.write(content)

print("Applied tweaks correctly")
