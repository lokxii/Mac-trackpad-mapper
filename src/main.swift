import Cocoa
import Foundation
import SwiftUI

func alert(msg: String = "") {
    let alert = NSAlert.init()
    alert.messageText = msg
    alert.addButton(withTitle: "OK")
    alert.runModal()
}

var settings: Settings = Settings()

func main() {
    let _ = NSApplication.shared
    NSApp.setActivationPolicy(.regular)
    NSApp.activate(ignoringOtherApps: true)

    // create status bar item
    let statusItem = NSStatusBar.system.statusItem(
                        withLength: NSStatusItem.variableLength)
    statusItem.button?.image = Bundle.main.image(forResource: "trackpad_status_icon")!
    statusItem.button?.image!.isTemplate = true

    // building menu
    let menu = MainMenu()

    // Hook menu to status bar item
    statusItem.menu = menu

    // Atart app
    NSApp.run()
}

main()
