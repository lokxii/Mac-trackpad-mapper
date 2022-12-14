import SwiftUI
import Cocoa

class PreferenceViewController: NSViewController {
    override func loadView() {
        title = "Preference"
        view = NSView(frame: NSMakeRect(0.0, 0.0, 300, 300))
        
        let ui = NSHostingView(rootView: PreferenceUIView())
        ui.translatesAutoresizingMaskIntoConstraints = false

        self.view.addSubview(ui)
        
        NSLayoutConstraint.activate([
            ui.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            ui.centerYAnchor.constraint(equalTo: view.centerYAnchor),
        ])
    }
}
