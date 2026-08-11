package com.example.chat.utils

import android.content.Context
import android.content.SharedPreferences

class ModerationManager(context: Context) {
    private val prefs: SharedPreferences = context.getSharedPreferences("moderation_prefs", Context.MODE_PRIVATE)
    private val ignoredUsers = mutableSetOf<String>()
    private val userViolations = mutableMapOf<String, Int>()

    init {
        val saved = prefs.getStringSet("ignored_users", emptySet())
        if (saved != null) {
            ignoredUsers.addAll(saved)
        }
    }

    private fun saveIgnoredUsers() {
        prefs.edit().putStringSet("ignored_users", ignoredUsers).apply()
    }

    fun isIgnored(userId: String): Boolean {
        return ignoredUsers.contains(userId)
    }

    fun filterProfanity(text: String, userId: String, currentUserId: String): String? {
        if (text.isEmpty()) return text
        if (userId == currentUserId) return text

        var needsSave = false

        // Level 1: Immediate Ban
        val level1Regex = "(死|氏|ﾀﾋ)(ね|んじまえ|ばいい)|(殺|56)(す|して)|キチ[ガガイ〇]イ|池沼|土人|チャンコロ|カタワ|メクラ|援助交際|パパ活|売春|強姦|自殺しろ|\\b(fuck|bitch|cunt|nigger|nigga|faggot)\\b|操你妈|傻逼|干你娘|肏你妈|씨발|개새끼|애미야".toRegex(RegexOption.IGNORE_CASE)
        
        if (level1Regex.containsMatchIn(text)) {
            if (!ignoredUsers.contains(userId)) {
                ignoredUsers.add(userId)
                needsSave = true
            }
            if (needsSave) saveIgnoredUsers()
            return null // Message dropped
        }

        // Level 2: Asterisk replacement and 3-strike rule
        val level2Regex = "まんこ|マンコ|おめこ|ペニス|うんこ|ウンコ|糞|マヌケ|キモイ|ウザい|\\b(shit|slut|whore|asshole|dick|cock|pussy)\\b|靠北|智障|脑残|鸡巴|牛逼|존나|병신|좆|지랄".toRegex(RegexOption.IGNORE_CASE)
        
        val filteredText = level2Regex.replace(text, "***")
        if (filteredText != text) {
            val violations = (userViolations[userId] ?: 0) + 1
            userViolations[userId] = violations
            if (violations >= 3) {
                if (!ignoredUsers.contains(userId)) {
                    ignoredUsers.add(userId)
                    needsSave = true
                }
            }
            if (needsSave) saveIgnoredUsers()
        }

        return filteredText
    }
}
