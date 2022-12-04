import Cocoa
import Foundation

class Menu: NSMenu {
	var process: Process? = nil

	@objc
	public func startProcess(_: Any?) {
		var processUrl = Bundle.main.bundleURL
		processUrl.appendPathComponent("Contents/MacOS/trackpad_mapper_util")
		process = try! Process.run(
			processUrl, arguments: [], terminationHandler: nil)

		removeItem(at: 1)
		let stopItem = NSMenuItem.init(
			title: "Stop absolute tracking",
			action: #selector(stopProcess(_:)),
			keyEquivalent: "s")
		stopItem.target = self
		insertItem(stopItem, at: 1)
	}

	@objc
	public func stopProcess(_: Any?) {
		if let process = process {
			process.terminate()
			self.process = nil

			removeItem(at: 1)
			let resumeItem = NSMenuItem.init(
				title: "Start absolute tracking",
				action: #selector(startProcess(_:)),
				keyEquivalent: "s")
			resumeItem.target = self
			insertItem(resumeItem, at: 1)
		}
	}
	
	@objc
	public func terminate(_: Any?) {
		if let process = process {
			process.terminate()
		}
		NSApp.terminate(nil)
	}
}

// initialize NSApp
let _ = NSApplication.shared
NSApp.setActivationPolicy(.regular)
NSApp.activate(ignoringOtherApps: true)

// create status bar item
var statusItem = NSStatusBar.system.statusItem(
				 	withLength: NSStatusItem.variableLength)
var iconUrl = Bundle.main.bundleURL
// iconUrl.appendPathComponent("Contents/Resources/trackpad-status-icon.png")
// statusItem.button?.title = "Trackpad Mapper"
statusItem.button?.image = Bundle.main.image(forResource: "trackpad_status_icon")!
statusItem.button?.image!.isTemplate = true

// building menu
let menu = Menu()
// Version
menu.addItem(
	withTitle: "Version 0.0.1",
    action: nil,
	keyEquivalent: "")
// Start process
let startItem = NSMenuItem.init(
					title: "Start absolute tracking",
		            action: #selector(Menu.startProcess(_:)),
					keyEquivalent: "s")
startItem.target = menu
menu.insertItem(startItem, at: 1)
// Quit app
let quitItem = NSMenuItem.init(
					title: "Quit trackpad mapper",
				    action: #selector(Menu.terminate(_:)),
					keyEquivalent: "q")
quitItem.target = menu
menu.addItem(quitItem)

// Hook menu to status bar item
statusItem.menu = menu

// Atart app
NSApp.run()
