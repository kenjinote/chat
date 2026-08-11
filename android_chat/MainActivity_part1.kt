package com.example.chat

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.ClickableText
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.platform.LocalUriHandler
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.chat.network.ChatMessage
import com.example.chat.utils.TextProcessor
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.res.stringResource

val Win95Gray = Color(192, 192, 192)
val Win95White = Color(255, 255, 255)
val Win95Black = Color(0, 0, 0)
val Win95DarkGray = Color(128, 128, 128)
val Win95TitleBlue = Color(0xFF000080)
val Win95Blue = Color(0xFF0000FF)

val win95TextStyle = TextStyle(
    fontFamily = FontFamily.Monospace,
    fontSize = 20.sp,
    color = Win95Black
)

@Composable
fun Win95Panel(modifier: Modifier = Modifier, content: @Composable () -> Unit) {
    Box(
        modifier = modifier
            .background(Win95Gray)
            .drawBehind {
                val bw = 2.dp.toPx()
                drawRect(Win95DarkGray, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                drawRect(Win95DarkGray, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                drawRect(Win95White, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                drawRect(Win95White, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
            }
            .padding(2.dp)
    ) {
        content()
    }
}

@Composable
fun Win95Button(onClick: () -> Unit, modifier: Modifier = Modifier, checked: Boolean = false, content: @Composable () -> Unit) {
    Box(
        modifier = modifier
            .clickable(onClick = onClick)
            .background(if (checked) Win95White else Win95Gray)
            .drawBehind {
                val bw = 2.dp.toPx()
                val topColor = if (checked) Win95DarkGray else Win95White
                val bottomColor = if (checked) Win95White else Win95Black
                drawRect(topColor, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                drawRect(topColor, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                drawRect(bottomColor, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                drawRect(bottomColor, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
            }
            .padding(horizontal = 8.dp, vertical = 4.dp),
        contentAlignment = Alignment.Center
    ) {
        content()
    }
}

@Composable
fun Win95TextField(value: String, onValueChange: (String) -> Unit, modifier: Modifier = Modifier, textStyle: TextStyle = win95TextStyle) {
    Box(
        modifier = modifier
            .height(60.dp)
            .background(Win95White)
            .drawBehind {
                val bw = 2.dp.toPx()
                drawRect(Win95DarkGray, topLeft = Offset(0f, 0f), size = Size(size.width, bw))
                drawRect(Win95DarkGray, topLeft = Offset(0f, 0f), size = Size(bw, size.height))
                drawRect(Win95White, topLeft = Offset(0f, size.height - bw), size = Size(size.width, bw))
                drawRect(Win95White, topLeft = Offset(size.width - bw, 0f), size = Size(bw, size.height))
            }
            .padding(4.dp)
    ) {
        BasicTextField(
            value = value,
            onValueChange = onValueChange,
            textStyle = textStyle,
            modifier = Modifier.fillMaxSize(),
            cursorBrush = SolidColor(Win95Black)
        )
    }
}

@Composable
fun Win95ComboBox(items: List<String>, selectedItem: String, onItemSelected: (String) -> Unit, modifier: Modifier = Modifier) {
    var expanded by remember { mutableStateOf(false) }
    Box(modifier = modifier) {
        Win95Button(onClick = { expanded = !expanded }) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(selectedItem, style = win95TextStyle)
                Spacer(modifier = Modifier.width(4.dp))
                Text("▼", style = win95TextStyle.copy(fontSize = 12.sp))
            }
        }
        DropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
            items.forEach { item ->
                DropdownMenuItem(
                    text = { Text(item, style = win95TextStyle) },
                    onClick = {
                        onItemSelected(item)
                        expanded = false
                    }
                )
            }
        }
    }
}

@Composable
fun AndroidChatTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = lightColorScheme(
            background = Win95Gray,
            surface = Win95Gray
        ),
        content = content
    )
}

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            AndroidChatTheme {
                Surface(modifier = Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.background) {
                    ChatScreen()
                }
            }
        }
    }
}
