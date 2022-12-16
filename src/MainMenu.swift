import Cocoa

class MainMenu: NSMenu {
	var process: Process? = nil
    var versionItem: NSMenuItem? = nil
    var startItem: NSMenuItem? = nil
    var stopItem: NSMenuItem? = nil
    var preferenceItem: NSMenuItem? = nil
    var quitItem: NSMenuItem? = nil

    let toggleTrackingItemIndex = 2
    let preferenceItemIndex = 3

    let preferenceWindow: NSWindow = NSWindow.init(contentViewController: PreferenceViewController.init())

    required init(coder: NSCoder) {
        super.init(coder: coder)
    }

    public init() {
        super.init(title: "")

        // Version
        versionItem = NSMenuItem.init(
            title: "Version 0.0.1",
            action: nil,
            keyEquivalent: "")

        // Start process
        startItem = NSMenuItem.init(
                            title: "Start absolute tracking",
                            action: #selector(MainMenu.startProcess(_:)),
                            keyEquivalent: "s")
        startItem!.target = self

        // Stop process
        stopItem = NSMenuItem.init(
                            title: "Stop absolute tracking",
                            action: #selector(MainMenu.stopProcess(_:)),
                            keyEquivalent: "s")
        stopItem!.target = self

        // Preference
        preferenceItem = NSMenuItem.init(
                            title: "Preference",
                            action: #selector(MainMenu.openPreference(_:)),
                            keyEquivalent: ",")
        preferenceItem!.target = self

        // Quit app
        quitItem = NSMenuItem.init(
                            title: "Quit trackpad mapper",
                            action: #selector(MainMenu.terminate(_:)),
                            keyEquivalent: "q")
        quitItem!.target = self

        addItem(versionItem!)
        addItem(NSMenuItem.separator())
        addItem(startItem!)
        addItem(preferenceItem!)
        addItem(NSMenuItem.separator())
        addItem(quitItem!)
    }

	@objc
	public func startProcess(_: Any?) {
        if process == nil {
            var processUrl = Bundle.main.bundleURL
            processUrl.appendPathComponent("Contents/MacOS/trackpad_mapper_util")
            do {
                process = try Process.run(
                    processUrl,
                    arguments: settings.useHeader ? [] : settings.toArgs(),
                    terminationHandler: nil)

                items[toggleTrackingItemIndex] = stopItem!
            } catch {
                alert(msg: "Cannot spawn process")
            }
        }
	}

	@objc
	public func stopProcess(_: Any?) {
		if let process = process {
			process.terminate()
			self.process = nil

            items[toggleTrackingItemIndex] = startItem!
		}
	}
	
	@objc
	public func terminate(_: Any?) {
		if let process = process {
			process.terminate()
		}
		NSApp.terminate(nil)
	}

    @objc
    public func openPreference(_: Any?) {
        preferenceWindow.makeKeyAndOrderFront(nil)
        preferenceWindow.orderedIndex = 0
    }
}
