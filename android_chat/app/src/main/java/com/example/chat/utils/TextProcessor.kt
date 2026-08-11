package com.example.chat.utils

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextDecoration
import androidx.compose.ui.text.withStyle

object TextProcessor {
    fun sanitizeText(input: String): String {
        val sb = java.lang.StringBuilder(input.length)
        for (c in input) {
            val codePoint = c.code
            
            // Combining Diacritical Marks
            if (codePoint in 0x0300..0x036F) continue
            if (codePoint in 0x1AB0..0x1AFF) continue
            if (codePoint in 0x1DC0..0x1DFF) continue
            if (codePoint in 0x20D0..0x20FF) continue
            if (codePoint in 0xFE20..0xFE2F) continue

            // Zero-width characters (excluding ZWJ which is needed for emojis)
            if (codePoint == 0x200B || codePoint == 0x200C || codePoint == 0xFEFF) continue

            // Bidi control characters
            if (codePoint in 0x202A..0x202E) continue
            if (codePoint in 0x2066..0x2069) continue

            // Block other zalgo blocks like 0x0483-0x0489
            if (codePoint in 0x0483..0x0489) continue

            // PUA (BMP)
            if (codePoint in 0xE000..0xF8FF) continue

            // Control & Spaces
            if (codePoint <= 0x001F && codePoint != 0x0A && codePoint != 0x0D && codePoint != 0x09) continue
            if (codePoint in 0x007F..0x009F) continue
            if (codePoint == 0xFFFE || codePoint == 0xFFFF) continue
            if (codePoint in 0xFFF9..0xFFFB) continue
            if (codePoint == 0x1680 || codePoint == 0x180E || codePoint == 0x2800 || codePoint == 0x3164) continue

            sb.append(c)
        }
        return sb.toString()
    }

    fun formatMessage(text: String): AnnotatedString {
        return buildAnnotatedString {
            // Very simple markdown parser for bold (**text** or *text*) and URL detection
            val urlPattern = "(https?://[\\w-]+(\\.[\\w-]+)+([\\w.,@?^=%&:/~+#-]*[\\w@?^=%&/~+#-])?)".toRegex()
            
            // First we process bold texts simply (this is a basic implementation, can be robust)
            // But for Compose, it's easier to find matches and add styles.
            var lastIndex = 0
            
            // Just handling URLs for now to keep it lightweight and fast
            val matches = urlPattern.findAll(text)
            for (match in matches) {
                val start = match.range.first
                val end = match.range.last + 1
                
                // Add text before URL
                if (start > lastIndex) {
                    appendBoldHandled(text.substring(lastIndex, start))
                }
                
                // Add URL with link style
                val url = match.value
                pushStringAnnotation(tag = "URL", annotation = url)
                withStyle(style = SpanStyle(color = Color.Blue, textDecoration = TextDecoration.Underline)) {
                    append(url)
                }
                pop()
                lastIndex = end
            }
            
            if (lastIndex < text.length) {
                appendBoldHandled(text.substring(lastIndex))
            }
        }
    }

    private fun AnnotatedString.Builder.appendBoldHandled(text: String) {
        // Simplified bold handler looking for *text*
        // Emojis shouldn't be bolded per Windows version spec, Compose handles emojis natively fine.
        val boldPattern = "\\*(.*?)\\*".toRegex()
        var lastIndex = 0
        val matches = boldPattern.findAll(text)
        for (match in matches) {
            val start = match.range.first
            val end = match.range.last + 1
            if (start > lastIndex) {
                append(text.substring(lastIndex, start))
            }
            withStyle(style = SpanStyle(fontWeight = FontWeight.Bold)) {
                append(match.groupValues[1])
            }
            lastIndex = end
        }
        if (lastIndex < text.length) {
            append(text.substring(lastIndex))
        }
    }
}
