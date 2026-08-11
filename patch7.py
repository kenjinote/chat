import re

# 1. Update NetworkClient.kt
nc_path = r"C:\Users\kenjinote\Desktop\chat\android_chat\app\src\main\java\com\example\chat\network\NetworkClient.kt"
with open(nc_path, "r", encoding="utf-8") as f:
    nc = f.read()

room_item_code = """@Serializable
data class RoomItem(
    val name: String,
    val count: Int
)

@Serializable
data class UserItem"""
nc = nc.replace("@Serializable\ndata class UserItem", room_item_code)

if "val rooms: List<RoomItem>? = null" not in nc:
    nc = nc.replace("val users: List<UserItem>? = null", "val users: List<UserItem>? = null,\n    val rooms: List<RoomItem>? = null")

with open(nc_path, "w", encoding="utf-8") as f:
    f.write(nc)

# 2. Update ChatViewModel.kt
cvm_path = r"C:\Users\kenjinote\Desktop\chat\android_chat\app\src\main\java\com\example\chat\ChatViewModel.kt"
with open(cvm_path, "r", encoding="utf-8") as f:
    cvm = f.read()

if "val roomList: StateFlow<List<RoomItem>>" not in cvm:
    # Add roomList
    cvm = cvm.replace("    private val _users = MutableStateFlow<List<UserItem>>(emptyList())",
                      "    private val _roomList = MutableStateFlow<List<RoomItem>>(emptyList())\n    val roomList: StateFlow<List<RoomItem>> = _roomList.asStateFlow()\n\n    private val _users = MutableStateFlow<List<UserItem>>(emptyList())")
    
    # Add roomList import
    cvm = cvm.replace("import com.example.chat.network.UserItem", "import com.example.chat.network.UserItem\nimport com.example.chat.network.RoomItem")

    # Add room_list handler
    handler_code = """            "welcome" -> {
                message.userId?.let { 
                    myUserId = it
                    _users.value = sortUsers(userMap.values.toList())
                }
            }
            "room_list" -> {
                message.rooms?.let {
                    _roomList.value = it
                }
            }"""
    cvm = cvm.replace("""            "welcome" -> {
                message.userId?.let { 
                    myUserId = it
                    _users.value = sortUsers(userMap.values.toList())
                }
            }""", handler_code)

    # Add moveRoom and requestRooms
    move_room_code = """
    fun requestRooms() {
        val msg = ChatMessage(type = "get_rooms")
        networkClient.sendTcpMessage(msg)
    }

    fun moveRoom(roomName: String, isPrivate: Boolean) {
        val joinMsg = ChatMessage(
            type = "join", 
            userName = myUserName, 
            secretId = myUserId, 
            pub = !isPrivate, 
            roomName = roomName, 
            nclr = -1
        )
        networkClient.sendTcpMessage(joinMsg)
    }
"""
    cvm = cvm.replace("    fun disconnect() {", move_room_code + "    fun disconnect() {")

with open(cvm_path, "w", encoding="utf-8") as f:
    f.write(cvm)

# 3. Update MainActivity.kt
ma_path = r"C:\Users\kenjinote\Desktop\chat\android_chat\app\src\main\java\com\example\chat\MainActivity.kt"
with open(ma_path, "r", encoding="utf-8") as f:
    ma = f.read()

# Add dialog imports
if "androidx.compose.ui.window.Dialog" not in ma:
    imports = """import androidx.compose.ui.window.Dialog
import androidx.compose.material3.Checkbox
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.ui.graphics.SolidColor
"""
    ma = ma.replace("import androidx.compose.ui.unit.sp\n", "import androidx.compose.ui.unit.sp\n" + imports)

# Add showMoveRoomDialog state
if "var showMoveRoomDialog by remember { mutableStateOf(false) }" not in ma:
    ma = ma.replace("    val messages by viewModel.messages.collectAsState()",
                    "    var showMoveRoomDialog by remember { mutableStateOf(false) }\n    val messages by viewModel.messages.collectAsState()")

# Update Move Room button
ma = ma.replace("""Win95Button(onClick = { /* Move Room */ }, modifier = Modifier.fillMaxWidth()) {
                        Text(stringResource(R.string.move_room_btn), style = win95TextStyle)
                    }""", """Win95Button(onClick = { 
                        showMoveRoomDialog = true
                        viewModel.requestRooms()
                    }, modifier = Modifier.fillMaxWidth()) {
                        Text(stringResource(R.string.move_room_btn), style = win95TextStyle)
                    }""")

# Add MoveRoomDialog call
if "if (showMoveRoomDialog)" not in ma:
    dialog_code = """
            if (showMoveRoomDialog) {
                MoveRoomDialog(
                    viewModel = viewModel,
                    onDismiss = { showMoveRoomDialog = false },
                    onConfirm = { roomName, isPrivate ->
                        viewModel.moveRoom(roomName, isPrivate)
                        showMoveRoomDialog = false
                    }
                )
            }
"""
    ma = ma.replace("            // Toolbar Area", dialog_code + "            // Toolbar Area")

# Add MoveRoomDialog composable
if "fun MoveRoomDialog(" not in ma:
    dialog_composable = """
@Composable
fun MoveRoomDialog(viewModel: ChatViewModel, onDismiss: () -> Unit, onConfirm: (String, Boolean) -> Unit) {
    var roomNameInput by remember { mutableStateOf("") }
    var isPrivate by remember { mutableStateOf(false) }
    val roomList by viewModel.roomList.collectAsState()
    
    Dialog(onDismissRequest = onDismiss) {
        Win95Panel(modifier = Modifier.width(320.dp).padding(4.dp)) {
            Column(modifier = Modifier.padding(8.dp)) {
                Win95Panel(modifier = Modifier.fillMaxWidth().background(Win95White), isRaised = false) {
                    BasicTextField(
                        value = roomNameInput,
                        onValueChange = { roomNameInput = it },
                        modifier = Modifier.fillMaxWidth().padding(4.dp),
                        textStyle = win95TextStyle,
                        singleLine = true,
                        cursorBrush = SolidColor(Win95Black)
                    )
                }
                Spacer(modifier = Modifier.height(8.dp))
                
                Win95Panel(modifier = Modifier.fillMaxWidth().height(150.dp).background(Win95White), isRaised = false) {
                    LazyColumn(modifier = Modifier.fillMaxSize().padding(4.dp)) {
                        items(roomList) { room ->
                            Text(
                                text = "${room.name} (${room.count})",
                                style = win95TextStyle,
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .clickable { roomNameInput = room.name }
                                    .padding(vertical = 2.dp)
                            )
                        }
                    }
                }
                Spacer(modifier = Modifier.height(8.dp))
                
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(checked = isPrivate, onCheckedChange = { isPrivate = it })
                    Text(stringResource(R.string.private_room), style = win95TextStyle)
                }
                
                Spacer(modifier = Modifier.height(8.dp))
                
                Row(horizontalArrangement = Arrangement.End, modifier = Modifier.fillMaxWidth()) {
                    Win95Button(onClick = { onConfirm(roomNameInput, isPrivate) }) {
                        Text("OK", style = win95TextStyle)
                    }
                    Spacer(modifier = Modifier.width(8.dp))
                    Win95Button(onClick = onDismiss) {
                        Text("Cancel", style = win95TextStyle)
                    }
                }
            }
        }
    }
}
"""
    ma += dialog_composable

with open(ma_path, "w", encoding="utf-8") as f:
    f.write(ma)

print("Patch 7 applied!")
