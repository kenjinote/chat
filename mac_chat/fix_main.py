import re

with open("main.mm", "r") as f:
    content = f.read()

# Fix 1: The duplicated updatePublicRooms fragment around line 350
content = re.sub(
r"""            \[self\.allRooms addObject:@\{@"display": \[NSString stringWithFormat:@"%@ \(%d\)", name, count\], @"name": name\}\];
        \}
    \}
    \[self filterRooms\];
\}

- \(void\)controlTextDidChange:\(NSNotification \*\)obj \{
    \[self filterRooms\];
\}

- \(void\)filterRooms \{
            \[self\.filteredRooms addObject:r\];
        \}
    \}
    std::cout << "\[DEBUG\] filterRooms filtered count: " << self\.filteredRooms\.count << std::endl;
    \[self\.roomTableView reloadData\];
    \[self\.roomTableView setNeedsDisplay:YES\];
\}
        \}
    \}
    return self\.filteredRooms\.count;
\}""",
r"""
- (void)controlTextDidChange:(NSNotification *)obj {
    [self filterRooms];
}

- (void)filterRooms {
    [self.filteredRooms removeAllObjects];
    NSString *query = self.searchField.stringValue.lowercaseString;
    for (NSDictionary *r in self.allRooms) {
        NSString *name = r[@"name"];
        if (query.length == 0 || [name.lowercaseString containsString:query]) {
            [self.filteredRooms addObject:r];
        }
    }
    std::cout << "[DEBUG] filterRooms filtered count: " << self.filteredRooms.count << std::endl;
    [self.roomTableView reloadData];
    [self.roomTableView setNeedsDisplay:YES];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return self.filteredRooms.count;
}
""", content)

with open("main.mm", "w") as f:
    f.write(content)
print("Fixed chunk 1")
