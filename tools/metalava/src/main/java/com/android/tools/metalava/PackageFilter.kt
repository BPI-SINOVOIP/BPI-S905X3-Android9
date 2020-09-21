package com.android.tools.metalava

import com.android.tools.metalava.model.PackageItem

class PackageFilter(
    val packagePrefixes: MutableList<String> = mutableListOf()
) {

    fun matches(qualifiedName: String): Boolean {
        for (prefix in packagePrefixes) {
            if (qualifiedName.startsWith(prefix)) {
                if (qualifiedName == prefix) {
                    return true
                } else if (qualifiedName[prefix.length] == '.') {
                    return true
                }
            }
        }

        return false
    }

    fun matches(packageItem: PackageItem): Boolean {
        return matches(packageItem.qualifiedName())
    }
}