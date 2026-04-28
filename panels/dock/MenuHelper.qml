// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma Singleton
import QtQuick
import Qt.labs.platform

Item {
    property Menu activeMenu: null
    
    signal menuClosed()
    Connections {
        target: activeMenu
        function onAboutToHide() {
            console.warn("[MenuHelper] activeMenu aboutToHide, menu:", activeMenu)
            activeMenu = null
            menuClosed()
        }
    }
    function openMenu(menu: Menu) {
        console.warn("[MenuHelper] openMenu called, menu:", menu, "activeMenu:", activeMenu)
        if (activeMenu) {
            console.warn("[MenuHelper] closing existing activeMenu:", activeMenu)
            activeMenu.close()
        }
        if (!menu) {
            console.warn("[MenuHelper] openMenu: menu is null, cannot open!")
            return
        }
        menu.open()
        activeMenu = menu
        console.warn("[MenuHelper] menu opened, activeMenu set to:", activeMenu)
    }
    function closeMenu(menu: Menu) {
        console.warn("[MenuHelper] closeMenu called, menu:", menu)
        menu.close()
    }
    function closeCurrent() {
        console.warn("[MenuHelper] closeCurrent called, activeMenu:", activeMenu)
        if (activeMenu) {
           closeMenu(activeMenu)
        }
    }
}
